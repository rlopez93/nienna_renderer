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

auto toGlm(const fastgltf::math::fvec3 &vector) -> glm::vec3
{
    return {vector.x(), vector.y(), vector.z()};
}

auto toGlm(const fastgltf::math::fvec4 &vector) -> glm::vec4
{
    return {vector.x(), vector.y(), vector.z(), vector.w()};
}

auto toGlm(const fastgltf::math::fquat &quat) -> glm::quat
{
    return {quat.w(), quat.x(), quat.y(), quat.z()};
}

auto parseGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset
{
    const auto supportedExtensions = fastgltf::Extensions::KHR_texture_transform
                                   | fastgltf::Extensions::KHR_materials_unlit
                                   | fastgltf::Extensions::KHR_lights_punctual
                                   | fastgltf::Extensions::KHR_materials_specular
                                   | fastgltf::Extensions::KHR_materials_ior
                                   | fastgltf::Extensions::KHR_materials_clearcoat;

    fastgltf::Parser parser{supportedExtensions};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                               | fastgltf::Options::LoadExternalBuffers
                               | fastgltf::Options::DecomposeNodeMatrices
                               | fastgltf::Options::GenerateMeshIndices;

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(gltfPath);

    if (!bool(gltfFile)) {
        throw std::runtime_error("parseGltfAsset failed");
    }

    auto loaded = parser.loadGltf(gltfFile.get(), gltfPath.parent_path(), gltfOptions);

    if (loaded.error() != fastgltf::Error::None) {
        throw std::runtime_error("parseGltfAsset failed");
    }

    return std::move(loaded.get());
}

auto makePixel(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha) -> std::vector<std::byte>
{
    return {
        static_cast<std::byte>(red),
        static_cast<std::byte>(green),
        static_cast<std::byte>(blue),
        static_cast<std::byte>(alpha),
    };
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

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy    = 1.0f;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp     = vk::CompareOp::eNever;

    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    samplerInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

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

auto mapMagFilter(const fastgltf::Optional<fastgltf::Filter> &magFilter) -> vk::Filter
{
    if (!magFilter.has_value()) {
        return vk::Filter::eLinear;
    }

    switch (*magFilter) {
    case fastgltf::Filter::Nearest:
        return vk::Filter::eNearest;
    case fastgltf::Filter::Linear:
        return vk::Filter::eLinear;

    // glTF disallows mipmapped enums for magFilter, but we assume
    // well-formed; fall back defensively without extra checks.
    default:
        return vk::Filter::eLinear;
    }
}

auto mapMinFilter(
    const fastgltf::Optional<fastgltf::Filter> &minFilter,
    vk::Filter                                 &outMinFilter,
    vk::SamplerMipmapMode                      &outMipmapMode) -> void
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
    const fastgltf::Optional<std::size_t> &samplerIndex,
    const fastgltf::Asset                 &gltfAsset) -> vk::SamplerCreateInfo
{
    auto samplerInfo = makeDefaultSamplerInfo();

    if (!samplerIndex.has_value()) {
        return samplerInfo;
    }

    const fastgltf::Sampler &sampler = gltfAsset.samplers[*samplerIndex];

    samplerInfo.addressModeU = mapWrap(sampler.wrapS);
    samplerInfo.addressModeV = mapWrap(sampler.wrapT);
    samplerInfo.addressModeW = samplerInfo.addressModeU;

    samplerInfo.magFilter = mapMagFilter(sampler.magFilter);

    vk::Filter            minFilter  = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;

    mapMinFilter(sampler.minFilter, minFilter, mipmapMode);

    samplerInfo.minFilter  = minFilter;
    samplerInfo.mipmapMode = mipmapMode;

    samplerInfo.pNext = nullptr;

    // TODO: Sampler caching (deferred).
    return samplerInfo;
}

struct DecodedImage {
    std::vector<std::byte> rgba8;
    vk::Extent2D           extent{0u, 0u};
};

auto decodeImageFromFile(const std::filesystem::path &path) -> DecodedImage
{
    int width    = 0;
    int height   = 0;
    int channels = 0;

    unsigned char *stbiData =
        stbi_load(path.string().c_str(), &width, &height, &channels, 4);

    if (stbiData == nullptr) {
        throw std::runtime_error("decodeImage failed");
    }

    const std::size_t byteCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;

    DecodedImage decoded{};
    decoded.extent = vk::Extent2D{
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
    };

    decoded.rgba8.resize(byteCount);

    std::memcpy(decoded.rgba8.data(), stbiData, byteCount);

    stbi_image_free(stbiData);

    return decoded;
}

auto decodeImageFromMemory(
    const unsigned char *data,
    int                  dataSize) -> DecodedImage
{
    int width    = 0;
    int height   = 0;
    int channels = 0;

    unsigned char *stbiData =
        stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);

    if (stbiData == nullptr) {
        throw std::runtime_error("decodeImage failed");
    }

    const std::size_t byteCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;

    DecodedImage decoded{};
    decoded.extent = vk::Extent2D{
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
    };

    decoded.rgba8.resize(byteCount);

    std::memcpy(decoded.rgba8.data(), stbiData, byteCount);

    stbi_image_free(stbiData);

    return decoded;
}

struct BytesView {
    const std::byte *data = nullptr;
    std::size_t      size = 0u;
};

auto getBufferBytes(
    const fastgltf::Asset &gltfAsset,
    std::size_t            bufferIndex) -> BytesView
{
    const fastgltf::Buffer &buffer = gltfAsset.buffers[bufferIndex];

    const auto visitor = overloads{
        [&](const fastgltf::sources::Vector &vector) -> BytesView {
            return {
                vector.bytes.data(),
                vector.bytes.size(),
            };
        },
        [&](const fastgltf::sources::ByteView &byteView) -> BytesView {
            return {
                byteView.bytes.data(),
                byteView.bytes.size(),
            };
        },
        [&](const fastgltf::sources::Array &array) -> BytesView {
            return {
                array.bytes.data(),
                array.bytes.size(),
            };
        },
        [&](const auto &) -> BytesView {
            throw std::runtime_error("buffer bytes unsupported");
        },
    };

    return buffer.data.visit(visitor);
}

auto decodeImage(
    const fastgltf::Asset       &gltfAsset,
    const fastgltf::Image       &image,
    const std::filesystem::path &directory) -> DecodedImage
{
    const auto visitor = overloads{
        [&](const fastgltf::sources::URI &uri) -> DecodedImage {
            if (!uri.uri.isLocalPath()) {
                throw std::runtime_error("image URI unsupported");
            }

            if (uri.fileByteOffset != 0u) {
                throw std::runtime_error("image URI offset");
            }

            const auto path = directory / uri.uri.fspath();
            return decodeImageFromFile(path);
        },
        [&](const fastgltf::sources::Vector &vector) -> DecodedImage {
            const auto *data =
                reinterpret_cast<const unsigned char *>(vector.bytes.data());

            const int size = static_cast<int>(vector.bytes.size());

            return decodeImageFromMemory(data, size);
        },
        [&](const fastgltf::sources::ByteView &byteView) -> DecodedImage {
            const auto *data =
                reinterpret_cast<const unsigned char *>(byteView.bytes.data());

            const int size = static_cast<int>(byteView.bytes.size());

            return decodeImageFromMemory(data, size);
        },
        [&](const fastgltf::sources::BufferView &view) -> DecodedImage {
            const fastgltf::BufferView &bufferView =
                gltfAsset.bufferViews[view.bufferViewIndex];

            const BytesView bufferBytes =
                getBufferBytes(gltfAsset, bufferView.bufferIndex);

            const std::size_t offset = bufferView.byteOffset;
            const std::size_t length = bufferView.byteLength;

            const auto *data =
                reinterpret_cast<const unsigned char *>(bufferBytes.data + offset);

            const int size = static_cast<int>(length);

            return decodeImageFromMemory(data, size);
        },
        [&](const auto &) -> DecodedImage {
            throw std::runtime_error("image source unsupported");
        },
    };

    return image.data.visit(visitor);
}

auto makeDefaultTextures(Asset &asset) -> void
{
    const auto defaultSamplerInfo = makeDefaultSamplerInfo();

    {
        Texture texture{};
        texture.extent      = vk::Extent2D{1u, 1u};
        texture.rgba8       = makePixel(255u, 255u, 255u, 255u);
        texture.samplerInfo = defaultSamplerInfo;
        asset.textures.push_back(std::move(texture));
    }

    {
        Texture texture{};
        texture.extent      = vk::Extent2D{1u, 1u};
        texture.rgba8       = makePixel(255u, 255u, 255u, 255u);
        texture.samplerInfo = defaultSamplerInfo;
        asset.textures.push_back(std::move(texture));
    }

    {
        Texture texture{};
        texture.extent      = vk::Extent2D{1u, 1u};
        texture.rgba8       = makePixel(128u, 128u, 255u, 255u);
        texture.samplerInfo = defaultSamplerInfo;
        asset.textures.push_back(std::move(texture));
    }
}

auto makeTextureRef(
    const fastgltf::TextureInfo      &textureInfo,
    const std::vector<std::uint32_t> &texRemap) -> TextureRef
{
    TextureRef textureRef{};

    textureRef.textureIndex = texRemap[textureInfo.textureIndex];

    textureRef.texCoord = static_cast<std::uint32_t>(textureInfo.texCoordIndex);

    // TODO: KHR_texture_transform (deferred).
    textureRef.uvTransform.enabled = VK_FALSE;

    return textureRef;
}

auto loadTextures(
    Asset                       &asset,
    const fastgltf::Asset       &gltfAsset,
    const std::filesystem::path &directory,
    std::vector<std::uint32_t>  &texRemap) -> void
{
    texRemap.clear();
    texRemap.resize(gltfAsset.textures.size(), 0u);

    for (std::size_t textureIndex = 0u; textureIndex < gltfAsset.textures.size();
         ++textureIndex) {

        const fastgltf::Texture &gltfTexture = gltfAsset.textures[textureIndex];

        if (!gltfTexture.imageIndex.has_value()) {
            throw std::runtime_error("texture missing image");
        }

        const fastgltf::Image &image = gltfAsset.images[*gltfTexture.imageIndex];

        DecodedImage decoded = decodeImage(gltfAsset, image, directory);

        Texture texture{};
        texture.extent = decoded.extent;
        texture.rgba8  = std::move(decoded.rgba8);

        texture.samplerInfo = makeSamplerInfo(gltfTexture.samplerIndex, gltfAsset);

        texture.samplerInfo.pNext = nullptr;

        texRemap[textureIndex] = static_cast<std::uint32_t>(asset.textures.size());

        asset.textures.push_back(std::move(texture));
    }
}

auto loadMaterials(
    Asset                            &asset,
    const fastgltf::Asset            &gltfAsset,
    const std::vector<std::uint32_t> &texRemap,
    std::vector<std::uint32_t>       &matRemap) -> void
{
    asset.materials.clear();

    {
        Material defaultMaterial{};
        asset.materials.push_back(defaultMaterial);
    }

    matRemap.clear();
    matRemap.resize(gltfAsset.materials.size(), 0u);

    for (std::size_t materialIndex = 0u; materialIndex < gltfAsset.materials.size();
         ++materialIndex) {

        const fastgltf::Material &gltfMaterial = gltfAsset.materials[materialIndex];

        Material material{};

        material.core.baseColorFactor = toGlm(gltfMaterial.pbrData.baseColorFactor);

        material.core.metallicFactor = gltfMaterial.pbrData.metallicFactor;

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
            const auto &info            = *gltfMaterial.normalTexture;
            material.core.normalTexture = makeTextureRef(
                fastgltf::TextureInfo{info.textureIndex, info.texCoordIndex},
                texRemap);
            material.core.normalScale = info.scale;
        }

        if (gltfMaterial.occlusionTexture.has_value()) {
            const auto &info               = *gltfMaterial.occlusionTexture;
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
            material.core.alphaMaskEnable  = VK_FALSE;
            material.core.alphaBlendEnable = VK_FALSE;
            break;
        case fastgltf::AlphaMode::Mask:
            material.core.alphaMaskEnable  = VK_TRUE;
            material.core.alphaBlendEnable = VK_FALSE;
            material.core.alphaCutoff      = gltfMaterial.alphaCutoff;
            break;
        case fastgltf::AlphaMode::Blend:
            material.core.alphaMaskEnable  = VK_FALSE;
            material.core.alphaBlendEnable = VK_TRUE;
            break;
        }

        material.core.doubleSided = gltfMaterial.doubleSided ? VK_TRUE : VK_FALSE;

        material.core.cullMode = gltfMaterial.doubleSided ? vk::CullModeFlagBits::eNone
                                                          : vk::CullModeFlagBits::eBack;

        // TODO: Extension extraction (deferred).
        material.unlit.enabled = gltfMaterial.unlit ? VK_TRUE : VK_FALSE;

        matRemap[materialIndex] = static_cast<std::uint32_t>(asset.materials.size());

        asset.materials.push_back(std::move(material));
    }
}

auto loadMeshes(
    Asset                            &asset,
    const fastgltf::Asset            &gltfAsset,
    const std::vector<std::uint32_t> &matRemap) -> void
{
    asset.meshes.clear();
    asset.meshes.resize(gltfAsset.meshes.size());

    for (std::size_t meshIndex = 0u; meshIndex < gltfAsset.meshes.size(); ++meshIndex) {
        const fastgltf::Mesh &gltfMesh = gltfAsset.meshes[meshIndex];
        Mesh                 &mesh     = asset.meshes[meshIndex];

        mesh.primitives.clear();
        mesh.primitives.reserve(gltfMesh.primitives.size());

        for (const fastgltf::Primitive &gltfPrimitive : gltfMesh.primitives) {
            if (gltfPrimitive.type != fastgltf::PrimitiveType::Triangles) {
                throw std::runtime_error("primitive type not TRIANGLES");
            }

            Primitive primitive{};
            primitive.topology = vk::PrimitiveTopology::eTriangleList;

            if (gltfPrimitive.materialIndex.has_value()) {
                primitive.materialIndex = matRemap[*gltfPrimitive.materialIndex];
            } else {
                primitive.materialIndex = 0u;
            }

            const auto &positionAccessor =
                gltfAsset
                    .accessors[gltfPrimitive.findAttribute("POSITION")->accessorIndex];
            primitive.vertices.resize(positionAccessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                gltfAsset,
                positionAccessor,
                [&](const glm::vec3 position, std::size_t idx) {
                    primitive.vertices[idx].position = position;
                });

            // indices assumed present (GenerateMeshIndices)
            const auto &indexAccessor =
                gltfAsset.accessors[gltfPrimitive.indicesAccessor.value()];
            primitive.indices.resize(indexAccessor.count);

            fastgltf::iterateAccessorWithIndex<std::uint16_t>(
                gltfAsset,
                indexAccessor,
                [&](std::uint16_t index, std::size_t idx) {
                    primitive.indices[idx] = index;
                });

            bool hasNormals  = false;
            bool hasTangents = false;
            for (auto &attribute : gltfPrimitive.attributes) {
                auto &accessor = gltfAsset.accessors[attribute.accessorIndex];

                if (attribute.name == "NORMAL") {
                    hasNormals = true;
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec3 normal, std::size_t idx) {
                            primitive.vertices[idx].normal = normal;
                        });
                }

                else if (attribute.name == "TANGENT") {
                    hasTangents = true;
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec4 tangent, std::size_t idx) {
                            primitive.vertices[idx].tangent = tangent;
                        });
                }

                else if (attribute.name == "TEXCOORD_0") {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec2 uv, std::size_t idx) {
                            primitive.vertices[idx].uv0 = uv;
                        });
                }

                else if (attribute.name == "TEXCOORD_1") {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        gltfAsset,
                        accessor,
                        [&](glm::vec2 uv, std::size_t idx) {
                            primitive.vertices[idx].uv1 = uv;
                        });
                }

                else if (attribute.name == "COLOR_0") {
                    if (accessor.type == fastgltf::AccessorType::Vec3) {
                        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            gltfAsset,
                            accessor,
                            [&](glm::vec3 color, std::size_t idx) {
                                primitive.vertices[idx].color = glm::vec4(color, 1.0f);
                            });
                    } else if (accessor.type == fastgltf::AccessorType::Vec4) {
                        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            gltfAsset,
                            accessor,
                            [&](glm::vec4 color, std::size_t idx) {
                                primitive.vertices[idx].color = color;
                            });
                    }
                }
            }

            primitive.tangentsValid = hasNormals && hasTangents;
            mesh.primitives.push_back(std::move(primitive));
        }
    }
}

auto loadCameras(
    Asset                 &asset,
    const fastgltf::Asset &gltfAsset) -> void
{
    asset.cameras.clear();
    asset.cameras.resize(gltfAsset.cameras.size());

    for (std::size_t cameraIndex = 0u; cameraIndex < gltfAsset.cameras.size();
         ++cameraIndex) {

        const fastgltf::Camera &gltfCamera = gltfAsset.cameras[cameraIndex];

        const auto visitor = overloads{
            [&](const fastgltf::Camera::Perspective &p) -> CameraModel {
                PerspectiveCamera perspectiveCamera{};
                perspectiveCamera.yfov        = p.yfov;
                perspectiveCamera.aspectRatio = p.aspectRatio;
                perspectiveCamera.znear       = p.znear;
                perspectiveCamera.zfar        = p.zfar;

                return perspectiveCamera;
            },
            [&](const fastgltf::Camera::Orthographic &o) -> CameraModel {
                OrthographicCamera orthographicCamera{};
                orthographicCamera.xmag  = o.xmag;
                orthographicCamera.ymag  = o.ymag;
                orthographicCamera.znear = o.znear;
                orthographicCamera.zfar  = o.zfar;
                return orthographicCamera;
            },
        };

        Camera camera{};
        camera.model = gltfCamera.camera.visit(visitor);

        asset.cameras[cameraIndex] = std::move(camera);
    }
}

auto loadNodes(
    Asset                 &asset,
    const fastgltf::Asset &gltfAsset) -> void
{
    asset.nodes.clear();
    asset.nodes.resize(gltfAsset.nodes.size());

    for (std::size_t nodeIndex = 0u; nodeIndex < gltfAsset.nodes.size(); ++nodeIndex) {

        const fastgltf::Node &gltfNode = gltfAsset.nodes[nodeIndex];

        Node node{};

        node.children.reserve(gltfNode.children.size());

        for (const std::size_t childIndex : gltfNode.children) {
            node.children.push_back(static_cast<std::uint32_t>(childIndex));
        }

        const auto visitor = overloads{
            [&](const fastgltf::TRS &trs) {
                node.translation = toGlm(trs.translation);
                node.rotation    = toGlm(trs.rotation);
                node.scale       = toGlm(trs.scale);
            },
            [&](const fastgltf::math::fmat4x4 &) {
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
    Asset                 &asset,
    const fastgltf::Asset &gltfAsset) -> void
{
    asset.scenes.clear();
    asset.scenes.resize(gltfAsset.scenes.size());

    for (std::size_t sceneIndex = 0u; sceneIndex < gltfAsset.scenes.size();
         ++sceneIndex) {

        const fastgltf::Scene &gltfScene = gltfAsset.scenes[sceneIndex];

        Scene scene{};
        scene.rootNodes.reserve(gltfScene.nodeIndices.size());

        for (const std::size_t nodeIndex : gltfScene.nodeIndices) {
            scene.rootNodes.push_back(static_cast<std::uint32_t>(nodeIndex));
        }

        asset.scenes[sceneIndex] = std::move(scene);
    }

    if (gltfAsset.defaultScene.has_value()) {
        asset.activeScene = static_cast<std::uint32_t>(*gltfAsset.defaultScene);
    } else {
        asset.activeScene = 0u;
    }
}

auto extractAsset(
    const fastgltf::Asset       &gltfAsset,
    const std::filesystem::path &directory) -> Asset
{
    Asset asset{};

    makeDefaultTextures(asset);

    std::vector<std::uint32_t> texRemap{};
    loadTextures(asset, gltfAsset, directory, texRemap);

    std::vector<std::uint32_t> matRemap{};
    loadMaterials(asset, gltfAsset, texRemap, matRemap);

    loadMeshes(asset, gltfAsset, matRemap);
    loadCameras(asset, gltfAsset);
    loadNodes(asset, gltfAsset);
    loadScenes(asset, gltfAsset);

    return asset;
}

auto postProcessAsset(Asset &) -> void
{
    // TODO:
    // - Generate flat normals when missing.
    // - Generate tangents (MikkTSpace) when needed.
    // - Validate missing-normal magnitude heuristic.
}

} // namespace

auto getAsset(const std::filesystem::path &gltfPath) -> Asset
{
    const fastgltf::Asset gltfAsset = parseGltfAsset(gltfPath);

    Asset asset = extractAsset(gltfAsset, gltfPath.parent_path());

    postProcessAsset(asset);

    return asset;
}
