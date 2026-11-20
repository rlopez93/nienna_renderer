#include "GltfLoader.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <stb_image.h>

namespace
{

template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

auto toGlm(fastgltf::math::fvec3 const& vector) -> glm::vec3
{
    return {vector.x(), vector.y(), vector.z()};
}

auto toGlm(fastgltf::math::fvec4 const& vector) -> glm::vec4
{
    return {vector.x(), vector.y(), vector.z(), vector.w()};
}

auto toGlm(fastgltf::math::fquat const& quat) -> glm::quat
{
    return {quat.w(), quat.x(), quat.y(), quat.z()};
}

auto parseGltfAsset(std::filesystem::path const& gltfPath) -> fastgltf::Asset
{
    auto const supportedExtensions =
        fastgltf::Extensions::KHR_materials_unlit
        | fastgltf::Extensions::KHR_materials_dispersion
        | fastgltf::Extensions::KHR_materials_volume
        | fastgltf::Extensions::KHR_materials_emissive_strength
        | fastgltf::Extensions::KHR_materials_transmission
        | fastgltf::Extensions::KHR_lights_punctual
        | fastgltf::Extensions::KHR_materials_specular
        | fastgltf::Extensions::KHR_materials_ior
        | fastgltf::Extensions::KHR_materials_clearcoat;

    auto parser = fastgltf::Parser{supportedExtensions};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                               | fastgltf::Options::LoadExternalBuffers
                               | fastgltf::Options::DecomposeNodeMatrices
                               | fastgltf::Options::GenerateMeshIndices;

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(gltfPath);

    if (!gltfFile) {
        throw std::runtime_error("parseGltfAsset failed");
    }

    auto loaded = parser.loadGltf(gltfFile.get(), gltfPath.parent_path(), gltfOptions);

    if (loaded.error() != fastgltf::Error::None) {
        throw std::runtime_error("parseGltfAsset failed");
    }

    return std::move(loaded.get());
}

auto makeDefaultSamplerInfo() -> vk::SamplerCreateInfo
{
    auto samplerInfo = vk::SamplerCreateInfo{};

    samplerInfo.pNext = nullptr;

    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    samplerInfo.mipLodBias = 0.0f;

    samplerInfo.anisotropyEnable = vk::False;
    samplerInfo.maxAnisotropy    = 1.0f;

    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp     = vk::CompareOp::eNever;

    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    samplerInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;

    samplerInfo.unnormalizedCoordinates = vk::False;

    return samplerInfo;
}

auto mapWrap(fastgltf::Wrap wrap) -> vk::SamplerAddressMode
{
    switch (wrap) {
    case fastgltf::Wrap::ClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    case fastgltf::Wrap::MirroredRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case fastgltf::Wrap::Repeat:
    default:
        return vk::SamplerAddressMode::eRepeat;
    }
}

auto mapFilter(fastgltf::Filter filter) -> vk::Filter
{
    switch (filter) {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return vk::Filter::eNearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return vk::Filter::eLinear;
    }
}

auto mapMagFilter(fastgltf::Optional<fastgltf::Filter> const& magFilter) -> vk::Filter
{
    if (!magFilter.has_value()) {
        return vk::Filter::eLinear;
    }

    switch (*magFilter) {
    case fastgltf::Filter::Nearest:
        return vk::Filter::eNearest;
    case fastgltf::Filter::Linear:
        return vk::Filter::eLinear;

    default:
        return vk::Filter::eLinear;
    }
}

auto mapMinFilter(
    fastgltf::Optional<fastgltf::Filter> const& minFilter,
    vk::Filter&                                 outMinFilter,
    vk::SamplerMipmapMode&                      outMipmapMode) -> void
{
    if (!minFilter.has_value()) {
        outMinFilter  = vk::Filter::eLinear;
        outMipmapMode = vk::SamplerMipmapMode::eLinear;
        return;
    }

    switch (*minFilter) {
    case fastgltf::Filter::Nearest:
        outMinFilter  = vk::Filter::eNearest;
        outMipmapMode = vk::SamplerMipmapMode::eNearest;
        return;

    case fastgltf::Filter::Linear:
        outMinFilter  = vk::Filter::eLinear;
        outMipmapMode = vk::SamplerMipmapMode::eNearest;
        return;

    case fastgltf::Filter::NearestMipMapNearest:
        outMinFilter  = vk::Filter::eNearest;
        outMipmapMode = vk::SamplerMipmapMode::eNearest;
        return;

    case fastgltf::Filter::LinearMipMapNearest:
        outMinFilter  = vk::Filter::eLinear;
        outMipmapMode = vk::SamplerMipmapMode::eNearest;
        return;

    case fastgltf::Filter::NearestMipMapLinear:
        outMinFilter  = vk::Filter::eNearest;
        outMipmapMode = vk::SamplerMipmapMode::eLinear;
        return;

    case fastgltf::Filter::LinearMipMapLinear:
        outMinFilter  = vk::Filter::eLinear;
        outMipmapMode = vk::SamplerMipmapMode::eLinear;
        return;

    default:
        outMinFilter  = vk::Filter::eLinear;
        outMipmapMode = vk::SamplerMipmapMode::eLinear;
        return;
    }
}

auto makeSamplerInfo(
    fastgltf::Optional<std::size_t> const& samplerIndex,
    fastgltf::Asset const&                 gltfAsset) -> vk::SamplerCreateInfo
{
    auto samplerInfo = makeDefaultSamplerInfo();

    if (!samplerIndex.has_value()) {
        return samplerInfo;
    }

    auto const& sampler = gltfAsset.samplers[*samplerIndex];

    samplerInfo.addressModeU = mapWrap(sampler.wrapS);
    samplerInfo.addressModeV = mapWrap(sampler.wrapT);
    samplerInfo.addressModeW = samplerInfo.addressModeU;

    samplerInfo.magFilter = mapMagFilter(sampler.magFilter);

    mapMinFilter(sampler.minFilter, samplerInfo.minFilter, samplerInfo.mipmapMode);

    return samplerInfo;
}

struct DecodedImage {
    std::vector<std::byte> rgba8;
    vk::Extent2D           extent{0u, 0u};
};

auto decodeImageData(
    int            width,
    int            height,
    unsigned char* stbiData) -> DecodedImage
{
    auto const byteCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;

    auto decoded = DecodedImage{};

    decoded.rgba8.resize(byteCount);

    decoded.extent = vk::Extent2D{
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
    };

    std::memcpy(decoded.rgba8.data(), stbiData, byteCount);

    stbi_image_free(stbiData);

    return decoded;
}

auto decodeImageFromFile(std::filesystem::path const& path) -> DecodedImage
{
    auto width    = 0;
    auto height   = 0;
    auto channels = 0;

    auto* stbiData = stbi_load(path.string().c_str(), &width, &height, &channels, 4);

    if (stbiData == nullptr) {
        throw std::runtime_error("decodeImage failed");
    }

    return decodeImageData(width, height, stbiData);
}

auto decodeImageFromMemory(
    unsigned char const* data,
    int                  dataSize) -> DecodedImage
{
    auto width    = 0;
    auto height   = 0;
    auto channels = 0;

    auto* stbiData =
        stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);

    if (stbiData == nullptr) {
        throw std::runtime_error("decodeImage failed");
    }

    return decodeImageData(width, height, stbiData);
}

struct BytesView {
    std::byte const* data = nullptr;
    std::size_t      size = 0u;
};

auto getBufferBytes(
    fastgltf::Asset const& gltfAsset,
    std::size_t            bufferIndex) -> BytesView
{
    auto const& buffer = gltfAsset.buffers[bufferIndex];

    auto const visitor = overloads{
        [&](fastgltf::sources::Vector const& vector) -> BytesView {
            return {
                vector.bytes.data(),
                vector.bytes.size(),
            };
        },
        [&](fastgltf::sources::ByteView const& byteView) -> BytesView {
            return {
                byteView.bytes.data(),
                byteView.bytes.size(),
            };
        },
        [&](fastgltf::sources::Array const& array) -> BytesView {
            return {
                array.bytes.data(),
                array.bytes.size(),
            };
        },
        [&](auto const&) -> BytesView {
            throw std::runtime_error("buffer bytes unsupported");
        },
    };

    return buffer.data.visit(visitor);
}

auto decodeImage(
    fastgltf::Asset const&       gltfAsset,
    fastgltf::Image const&       image,
    std::filesystem::path const& directory) -> DecodedImage
{
    auto const visitor = overloads{
        [&](fastgltf::sources::URI const& uri) -> DecodedImage {
            if (!uri.uri.isLocalPath()) {
                throw std::runtime_error("image URI unsupported");
            }

            if (uri.fileByteOffset != 0u) {
                throw std::runtime_error("image URI offset");
            }

            auto const path = directory / uri.uri.fspath();
            return decodeImageFromFile(path);
        },
        [&](fastgltf::sources::Vector const& vector) -> DecodedImage {
            auto const* data =
                reinterpret_cast<unsigned char const*>(vector.bytes.data());
            auto const size = static_cast<int>(vector.bytes.size());

            return decodeImageFromMemory(data, size);
        },
        [&](fastgltf::sources::ByteView const& byteView) -> DecodedImage {
            auto const* data =
                reinterpret_cast<unsigned char const*>(byteView.bytes.data());
            auto const size = static_cast<int>(byteView.bytes.size());

            return decodeImageFromMemory(data, size);
        },
        [&](fastgltf::sources::BufferView const& view) -> DecodedImage {
            auto const& bufferView  = gltfAsset.bufferViews[view.bufferViewIndex];
            auto const  bufferBytes = getBufferBytes(gltfAsset, bufferView.bufferIndex);
            auto const  offset      = bufferView.byteOffset;
            auto const  length      = bufferView.byteLength;

            auto const* data =
                reinterpret_cast<unsigned char const*>(bufferBytes.data + offset);
            auto const size = static_cast<int>(length);

            return decodeImageFromMemory(data, size);
        },
        [&](auto const&) -> DecodedImage {
            throw std::runtime_error("image source unsupported");
        },
    };

    return image.data.visit(visitor);
}

auto makeDefaultTextures(RenderAsset& asset) -> void
{
    auto const defaultSamplerInfo = makeDefaultSamplerInfo();

    {
        auto texture   = Texture{};
        texture.extent = vk::Extent2D{1u, 1u};
        texture.rgba8  = std::vector{
            std::byte{255u},
            std::byte{255u},
            std::byte{255u},
            std::byte{255u}};
        texture.samplerInfo = defaultSamplerInfo;
        asset.textures.push_back(std::move(texture));
    }

    {
        auto texture   = Texture{};
        texture.extent = vk::Extent2D{1u, 1u};
        texture.rgba8  = std::vector{
            std::byte{255u},
            std::byte{255u},
            std::byte{255u},
            std::byte{255u}};
        texture.samplerInfo = defaultSamplerInfo;
        asset.textures.push_back(std::move(texture));
    }

    {
        auto texture   = Texture{};
        texture.extent = vk::Extent2D{1u, 1u};
        texture.rgba8  = std::vector{
            std::byte{128u},
            std::byte{128u},
            std::byte{255u},
            std::byte{255u}};
        texture.samplerInfo = defaultSamplerInfo;
        asset.textures.push_back(std::move(texture));
    }
}

auto makeTextureRef(
    fastgltf::TextureInfo const&      textureInfo,
    std::vector<std::uint32_t> const& texRemap) -> TextureRef
{
    auto textureRef         = TextureRef{};
    textureRef.textureIndex = texRemap[textureInfo.textureIndex];
    textureRef.texCoord     = static_cast<std::uint32_t>(textureInfo.texCoordIndex);
    textureRef.uvTransform.enabled = vk::False;

    return textureRef;
}

auto loadTextures(
    RenderAsset&                 asset,
    fastgltf::Asset const&       gltfAsset,
    std::filesystem::path const& directory,
    std::vector<std::uint32_t>&  texRemap) -> void
{
    texRemap.clear();
    texRemap.resize(gltfAsset.textures.size(), 0u);

    for (std::size_t textureIndex = 0u; textureIndex < gltfAsset.textures.size();
         ++textureIndex) {

        auto const& gltfTexture = gltfAsset.textures[textureIndex];

        if (!gltfTexture.imageIndex.has_value()) {
            throw std::runtime_error("texture missing image");
        }

        auto const& image = gltfAsset.images[*gltfTexture.imageIndex];

        auto decoded = decodeImage(gltfAsset, image, directory);

        auto texture        = Texture{};
        texture.rgba8       = std::move(decoded.rgba8);
        texture.extent      = decoded.extent;
        texture.samplerInfo = makeSamplerInfo(gltfTexture.samplerIndex, gltfAsset);

        texRemap[textureIndex] = static_cast<std::uint32_t>(asset.textures.size());

        asset.textures.push_back(std::move(texture));
    }
}

auto loadMaterials(
    RenderAsset&                      asset,
    fastgltf::Asset const&            gltfAsset,
    std::vector<std::uint32_t> const& texRemap,
    std::vector<std::uint32_t>&       matRemap) -> void
{
    asset.materials.clear();
    asset.materials.emplace_back(); // default material

    matRemap.clear();
    matRemap.resize(gltfAsset.materials.size(), 0u);

    for (std::size_t materialIndex = 0u; materialIndex < gltfAsset.materials.size();
         ++materialIndex) {

        auto const& gltfMaterial = gltfAsset.materials[materialIndex];

        auto material                 = Material{};
        material.core.baseColorFactor = toGlm(gltfMaterial.pbrData.baseColorFactor);
        material.core.metallicFactor  = gltfMaterial.pbrData.metallicFactor;
        material.core.roughnessFactor = gltfMaterial.pbrData.roughnessFactor;

        if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
            material.core.baseColorTexture =
                makeTextureRef(*gltfMaterial.pbrData.baseColorTexture, texRemap);
        }

        if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value()) {
            material.core.metallicRoughnessTexture = makeTextureRef(
                *gltfMaterial.pbrData.metallicRoughnessTexture,
                texRemap);
        }

        if (gltfMaterial.normalTexture.has_value()) {
            auto const& info            = *gltfMaterial.normalTexture;
            material.core.normalTexture = makeTextureRef(
                fastgltf::TextureInfo{info.textureIndex, info.texCoordIndex},
                texRemap);
            material.core.normalScale = info.scale;
        }

        if (gltfMaterial.occlusionTexture.has_value()) {
            auto const& info               = *gltfMaterial.occlusionTexture;
            material.core.occlusionTexture = makeTextureRef(
                fastgltf::TextureInfo{info.textureIndex, info.texCoordIndex},
                texRemap);
            material.core.occlusionStrength = info.strength;
        }
        if (gltfMaterial.emissiveTexture.has_value()) {
            material.core.emissiveTexture =
                makeTextureRef(*gltfMaterial.emissiveTexture, texRemap);
        }

        material.core.emissiveFactor = toGlm(gltfMaterial.emissiveFactor);
        switch (gltfMaterial.alphaMode) {
        case fastgltf::AlphaMode::Opaque:
            material.core.alphaMaskEnable  = vk::False;
            material.core.alphaBlendEnable = vk::False;
            break;
        case fastgltf::AlphaMode::Mask:
            material.core.alphaMaskEnable  = vk::True;
            material.core.alphaBlendEnable = vk::False;
            material.core.alphaCutoff      = gltfMaterial.alphaCutoff;
            break;
        case fastgltf::AlphaMode::Blend:
            material.core.alphaMaskEnable  = vk::False;
            material.core.alphaBlendEnable = vk::True;
            break;
        }

        material.core.doubleSided = gltfMaterial.doubleSided ? vk::True : vk::False;

        material.core.cullMode = gltfMaterial.doubleSided ? vk::CullModeFlagBits::eNone
                                                          : vk::CullModeFlagBits::eBack;

        material.unlit.enabled = gltfMaterial.unlit ? vk::True : vk::False;

        matRemap[materialIndex] = static_cast<std::uint32_t>(asset.materials.size());

        asset.materials.push_back(std::move(material));
    }
}

auto loadMeshes(
    RenderAsset&                      asset,
    fastgltf::Asset const&            gltfAsset,
    std::vector<std::uint32_t> const& matRemap) -> void
{
    asset.meshes.clear();
    asset.meshes.resize(gltfAsset.meshes.size());

    for (std::size_t meshIndex = 0u; meshIndex < gltfAsset.meshes.size(); ++meshIndex) {
        auto const& gltfMesh = gltfAsset.meshes[meshIndex];
        auto&       mesh     = asset.meshes[meshIndex];

        mesh.submeshes.clear();
        mesh.submeshes.reserve(gltfMesh.primitives.size());

        for (fastgltf::Primitive const& gltfPrimitive : gltfMesh.primitives) {
            if (gltfPrimitive.type != fastgltf::PrimitiveType::Triangles) {
                throw std::runtime_error("primitive type not TRIANGLES");
            }

            auto submesh = Submesh{};

            if (gltfPrimitive.materialIndex.has_value()) {
                submesh.materialIndex = matRemap[*gltfPrimitive.materialIndex];
            } else {
                submesh.materialIndex = 0u;
            }

            auto const& positionAccessor =
                gltfAsset
                    .accessors[gltfPrimitive.findAttribute("POSITION")->accessorIndex];
            submesh.vertices.resize(positionAccessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                gltfAsset,
                positionAccessor,
                [&](glm::vec3 const position, std::size_t idx) -> void {
                    submesh.vertices[idx].position = position;
                });

            // indices assumed present (GenerateMeshIndices)
            auto const& indexAccessor =
                gltfAsset.accessors[gltfPrimitive.indicesAccessor.value()];
            submesh.indices.resize(indexAccessor.count);

            fastgltf::iterateAccessorWithIndex<std::uint32_t>(
                gltfAsset,
                indexAccessor,
                [&](std::uint32_t index, std::size_t idx) -> void {
                    submesh.indices[idx] = index;
                });

            auto hasNormals  = false;
            auto hasTangents = false;
            for (auto& attribute : gltfPrimitive.attributes) {
                auto& accessor = gltfAsset.accessors[attribute.accessorIndex];

                if (attribute.name == "NORMAL") {
                    hasNormals = true;
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec3 normal, std::size_t idx) -> void {
                            submesh.vertices[idx].normal = normal;
                        });
                }

                else if (attribute.name == "TANGENT") {
                    hasTangents = true;
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec4 tangent, std::size_t idx) -> void {
                            submesh.vertices[idx].tangent = tangent;
                        });
                }

                else if (attribute.name == "TEXCOORD_0") {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec2 uv, std::size_t idx) -> void {
                            submesh.vertices[idx].uv0 = uv;
                        });
                }

                else if (attribute.name == "TEXCOORD_1") {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec2 uv, std::size_t idx) -> void {
                            submesh.vertices[idx].uv1 = uv;
                        });
                }

                else if (attribute.name == "COLOR_0") {
                    if (accessor.type == fastgltf::AccessorType::Vec3) {
                        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            gltfAsset,
                            accessor,
                            [&](glm::vec3 color, std::size_t idx) -> void {
                                submesh.vertices[idx].color = glm::vec4(color, 1.0f);
                            });
                    } else if (accessor.type == fastgltf::AccessorType::Vec4) {
                        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            gltfAsset,
                            accessor,
                            [&](glm::vec4 color, std::size_t idx) -> void {
                                submesh.vertices[idx].color = color;
                            });
                    }
                }
            }

            submesh.tangentsValid = hasNormals && hasTangents;
            mesh.submeshes.push_back(std::move(submesh));
        }
    }
}

auto loadCameras(
    RenderAsset&           asset,
    fastgltf::Asset const& gltfAsset) -> void
{
    asset.cameras.clear();
    asset.cameras.resize(gltfAsset.cameras.size());

    auto const cameraVisitor = overloads{
        [&](fastgltf::Camera::Perspective const& p) -> CameraModel {
            PerspectiveCamera perspectiveCamera{};
            perspectiveCamera.yfov        = p.yfov;
            perspectiveCamera.aspectRatio = p.aspectRatio;
            perspectiveCamera.znear       = p.znear;
            perspectiveCamera.zfar        = p.zfar;

            return perspectiveCamera;
        },
        [&](fastgltf::Camera::Orthographic const& o) -> CameraModel {
            OrthographicCamera orthographicCamera{};
            orthographicCamera.xmag  = o.xmag;
            orthographicCamera.ymag  = o.ymag;
            orthographicCamera.znear = o.znear;
            orthographicCamera.zfar  = o.zfar;
            return orthographicCamera;
        },
    };

    for (std::size_t cameraIndex = 0u; cameraIndex < gltfAsset.cameras.size();
         ++cameraIndex) {

        auto const& gltfCamera = gltfAsset.cameras[cameraIndex];

        asset.cameras[cameraIndex] =
            Camera{.model = gltfCamera.camera.visit(cameraVisitor)};
    }

    // final default camera
    asset.cameras.push_back(Camera{.model = PerspectiveCamera{}});
}

auto loadNodes(
    RenderAsset&           asset,
    fastgltf::Asset const& gltfAsset) -> void
{
    asset.nodes.clear();
    asset.nodes.resize(gltfAsset.nodes.size());

    for (std::size_t nodeIndex = 0u; nodeIndex < gltfAsset.nodes.size(); ++nodeIndex) {

        auto const& gltfNode = gltfAsset.nodes[nodeIndex];

        auto node = SceneNode{};

        node.childNodeIndices.reserve(gltfNode.children.size());

        for (std::size_t const childIndex : gltfNode.children) {
            node.childNodeIndices.push_back(static_cast<std::uint32_t>(childIndex));
        }

        auto const visitor = overloads{
            [&](fastgltf::TRS const& trs) -> void {
                node.translation = toGlm(trs.translation);
                node.rotation    = toGlm(trs.rotation);
                node.scale       = toGlm(trs.scale);
            },
            [&](fastgltf::math::fmat4x4 const&) -> void {
                throw std::runtime_error("node transform not TRS");
            },
        };

        gltfNode.transform.visit(visitor);

        if (gltfNode.meshIndex.has_value()) {
            node.meshIndex = static_cast<std::uint32_t>(*gltfNode.meshIndex);
        }

        if (gltfNode.cameraIndex.has_value()) {
            node.cameraIndex = static_cast<std::uint32_t>(*gltfNode.cameraIndex);
        }

        asset.nodes[nodeIndex] = std::move(node);
    }
}

auto loadScenes(
    RenderAsset&           asset,
    fastgltf::Asset const& gltfAsset) -> void
{
    asset.scenes.clear();
    asset.scenes.resize(gltfAsset.scenes.size());

    for (std::size_t sceneIndex = 0u; sceneIndex < gltfAsset.scenes.size();
         ++sceneIndex) {

        auto const& gltfScene = gltfAsset.scenes[sceneIndex];

        auto sceneRoots = SceneRoots{};
        sceneRoots.rootNodeIndices.reserve(gltfScene.nodeIndices.size());

        for (std::size_t const nodeIndex : gltfScene.nodeIndices) {
            sceneRoots.rootNodeIndices.push_back(static_cast<std::uint32_t>(nodeIndex));
        }

        asset.scenes[sceneIndex] = std::move(sceneRoots);
    }

    if (gltfAsset.defaultScene.has_value()) {
        asset.activeScene = static_cast<std::uint32_t>(*gltfAsset.defaultScene);
    } else {
        asset.activeScene = 0u;
    }
}

auto extractAsset(
    fastgltf::Asset const&       gltfAsset,
    std::filesystem::path const& directory) -> RenderAsset
{
    auto asset = RenderAsset{};

    makeDefaultTextures(asset);

    auto texRemap = std::vector<std::uint32_t>{};
    loadTextures(asset, gltfAsset, directory, texRemap);

    auto matRemap = std::vector<std::uint32_t>{};
    loadMaterials(asset, gltfAsset, texRemap, matRemap);

    loadMeshes(asset, gltfAsset, matRemap);
    loadCameras(asset, gltfAsset);
    loadNodes(asset, gltfAsset);
    loadScenes(asset, gltfAsset);

    return asset;
}

auto postProcessAsset(RenderAsset&) -> void
{
    // TODO:
    // - Generate flat normals when missing.
    // - Generate tangents (MikkTSpace) when needed.
}

} // namespace

auto getAsset(std::filesystem::path const& gltfPath) -> RenderAsset
{
    auto const gltfAsset = parseGltfAsset(gltfPath);

    auto asset = extractAsset(gltfAsset, gltfPath.parent_path());

    postProcessAsset(asset);

    return asset;
}
