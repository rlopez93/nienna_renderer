#include "gltfLoader.hpp"
#include "Camera.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <stb_image.h>

#include <vector>

auto getGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset
{
    constexpr auto supportedExtensions = fastgltf::Extensions::KHR_mesh_quantization
                                       | fastgltf::Extensions::KHR_texture_transform
                                       | fastgltf::Extensions::KHR_materials_variants;

    fastgltf::Parser parser(supportedExtensions);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble
        | fastgltf::Options::
            LoadExternalBuffers //|
                                // fastgltf::Options::LoadExternalImages
        | fastgltf::Options::DecomposeNodeMatrices
        | fastgltf::Options::GenerateMeshIndices;

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    if (!bool(gltfFile)) {
        throw std::runtime_error{fmt::format(
            "Failed to open glTF file: {}",
            fastgltf::getErrorMessage(gltfFile.error()))};
    }

    auto asset = parser.loadGltf(gltfFile.get(), gltfPath.parent_path(), gltfOptions);

    if (fastgltf::getErrorName(asset.error()) != "None") {
        throw std::runtime_error{fmt::format(
            "Failed to open glTF file: {}",
            fastgltf::getErrorMessage(gltfFile.error()))};
    }

    return std::move(asset.get());
}

auto toGLM(const fastgltf::math::fmat4x4 &M) -> glm::mat4
{
    glm::mat4 N;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            N[i][j] = M[i][j];
        }
    }
    return N;
}

auto toGLM(const fastgltf::math::fquat &Q) -> glm::quat
{
    return {Q.w(), Q.x(), Q.y(), Q.z()};
}

auto toGLM(const fastgltf::math::fvec3 &V) -> glm::vec3
{
    return {V.x(), V.y(), V.z()};
}

auto toGLM(const fastgltf::math::fvec4 &V) -> glm::vec4
{
    return {V.x(), V.y(), V.z(), V.w()};
}

// helper type for the visitor
template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

auto getCamera(
    const fastgltf::Camera        &camera,
    const fastgltf::math::fmat4x4 &matrix) -> PerspectiveCamera
{
    fastgltf::math::fvec3 _scale;
    fastgltf::math::fquat _rotation;
    fastgltf::math::fvec3 _translation;

    fastgltf::math::decomposeTransformMatrix(matrix, _scale, _rotation, _translation);

    glm::quat rotation    = toGLM(_rotation);
    glm::vec3 translation = toGLM(_translation);

    const auto &perspectiveCamera =
        std::get<fastgltf::Camera::Perspective>(camera.camera);

    return {
        .translation = translation,
        .rotation    = rotation,
        .yfov        = perspectiveCamera.yfov,
        .aspectRatio = perspectiveCamera.aspectRatio.value_or(1.5f),
        .znear       = perspectiveCamera.znear,
        .zfar        = perspectiveCamera.zfar};
}

auto getTexture(
    const fastgltf::Asset       &asset,
    const fastgltf::TextureInfo &textureInfo,
    Scene                       &scene,
    const std::filesystem::path &directory)
{
    const auto textureIndex = scene.textures.size();

    auto &texture = asset.textures[textureInfo.textureIndex];
    auto &image   = asset.images[texture.imageIndex.value()];

    namespace sources  = fastgltf::sources;
    const auto visitor = overloads{
        [&directory, &scene](const sources::URI &imageURI) {
            auto path = directory / imageURI.uri.fspath();
            // fmt::println(stderr, "\"{}\"", path.c_str());
            int  x, y, n;
            auto stbiData = stbi_load(path.c_str(), &x, &y, &n, 4);

            // fmt::println(stderr, "channels in file: {}", n);
            assert(stbiData);

            scene.textures.push_back(
                Texture{
                    .name   = imageURI.uri.fspath().filename(),
                    .data   = std::vector(stbiData, stbiData + (x * y * 4)),
                    .extent = vk::Extent2D(x, y)});

            stbi_image_free(stbiData);
        },
        [](std::monostate) { throw std::logic_error{"std::monostate"}; },
        [](const sources::BufferView &view) {
            throw std::logic_error{"sources::BufferView"};
        },
        [](const sources::Array &data) { throw std::logic_error{"sources::Array"}; },
        [](const sources::Vector &data) { throw std::logic_error{"sources::Vector"}; },
        [](const sources::CustomBuffer &buf) {
            throw std::logic_error{"sources::CustomBuffer"};
        },
        [](const sources::ByteView &byteView) {
            throw std::logic_error{"sources::ByteView"};
        },
        [](const sources::Fallback &fallback) {
            throw std::logic_error{"sources::Fallback"};
        }};

    image.data.visit(visitor);

    auto sampler = [&texture, &asset] -> std::optional<fastgltf::Sampler> {
        if (texture.samplerIndex.has_value()) {
            return asset.samplers[texture.samplerIndex.value()];
        }

        else {
            return {};
        }
    };

    return textureIndex;
}

auto getMaterial(
    const fastgltf::Asset       &asset,
    const fastgltf::Primitive   &primitive,
    Scene                       &scene,
    Mesh                        &mesh,
    const std::filesystem::path &directory)
{
    const auto &material = asset.materials[primitive.materialIndex.value()];
    mesh.color           = toGLM(material.pbrData.baseColorFactor);
    // fmt::println(stderr, "{} {} {} {}", mesh.color.r, mesh.color.g, mesh.color.b,
    // mesh.color.a);

    if (material.pbrData.baseColorTexture.has_value()) {
        mesh.textureIndex = getTexture(
            asset,
            material.pbrData.baseColorTexture.value(),
            scene,
            directory);
    }
}
void getAttributes(
    const fastgltf::Asset     &asset,
    const fastgltf::Primitive &primitive,
    Mesh                      &mesh)
{
    {
        auto &positionAccessor =
            asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
        mesh.primitives.resize(positionAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset,
            positionAccessor,
            [&](glm::vec3 position, std::size_t idx) {
                mesh.primitives[idx].position = position;
            });
    }

    if (primitive.indicesAccessor.has_value()) {
        auto &indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
        mesh.indices.resize(indexAccessor.count);

        fastgltf::iterateAccessorWithIndex<std::uint16_t>(
            asset,
            indexAccessor,
            [&](std::uint16_t index, std::size_t idx) { mesh.indices[idx] = index; });
    }

    for (auto &attribute : primitive.attributes) {
        // fmt::println(stderr, "\n{}", attribute.name);

        auto &accessor = asset.accessors[attribute.accessorIndex];

        // switch (accessor.type) {
        //     using fastgltf::AccessorType;
        // case AccessorType::Invalid:
        //     fmt::println(stderr, "Invalid");
        //     break;
        // case AccessorType::Vec2:
        //     fmt::println(stderr, "Vec2");
        //     break;
        // case AccessorType::Vec3:
        //     fmt::println(stderr, "Vec3");
        //     break;
        // case AccessorType::Vec4:
        //     fmt::println(stderr, "Vec4");
        //     break;
        // case AccessorType::Mat2:
        //     fmt::println(stderr, "Mat2");
        //     break;
        // case AccessorType::Mat3:
        //     fmt::println(stderr, "Mat3");
        //     break;
        // case AccessorType::Mat4:
        //     fmt::println(stderr, "Mat4");
        //     break;
        // case AccessorType::Scalar:
        //     fmt::println(stderr, "Scalar");
        //     break;
        // }

        if (attribute.name == "NORMAL") {
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                accessor,
                [&](glm::vec3 normal, std::size_t idx) {
                    mesh.primitives[idx].normal = normal;
                });
        }

        else if (attribute.name == "TEXCOORD_0") {
            fastgltf::iterateAccessorWithIndex<glm::vec2>(
                asset,
                accessor,
                [&](glm::vec2 uv, std::size_t idx) {
                    // mesh.primitives[idx].uv[0] = uv;
                    mesh.primitives[idx].uv = uv;
                });
        }

        else if (attribute.name == "TEXCOORD_1") {
            fastgltf::iterateAccessorWithIndex<glm::vec2>(
                asset,
                accessor,
                [&](glm::vec2 uv, std::size_t idx) {
                    // mesh.primitives[idx].uv[1] = uv;
                });
        }

        else if (attribute.name == "COLOR_0") {
            // fmt::println(stderr, "FOUND COLOR\n");
            if (accessor.type == fastgltf::AccessorType::Vec3) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset,
                    accessor,
                    [&](glm::vec3 color, std::size_t idx) {
                        mesh.primitives[idx].color = glm::vec4(color, 1.0f);
                    });
            } else if (accessor.type == fastgltf::AccessorType::Vec4) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    asset,
                    accessor,
                    [&](glm::vec4 color, std::size_t idx) {
                        mesh.primitives[idx].color = color;
                    });
            }
        }
    }
}

auto getSceneData(
    const fastgltf::Asset       &asset,
    const std::filesystem::path &directory) -> Scene
{

    auto scene = Scene();

    fastgltf::iterateSceneNodes(
        asset,
        0,
        fastgltf::math::fmat4x4{},
        [&](const fastgltf::Node &node, const fastgltf::math::fmat4x4 &matrix) {
            // fmt::println(stderr, "Node: {}\n", node.name);
            if (node.cameraIndex.has_value()) {
                scene.cameras.emplace_back(
                    getCamera(asset.cameras[node.cameraIndex.value()], matrix));
            }

            if (node.meshIndex.has_value()) {
                auto &assetMesh  = asset.meshes[node.meshIndex.value()];
                auto &mesh       = scene.meshes.emplace_back();
                mesh.modelMatrix = toGLM(matrix);

                // fmt::println(stderr, "Mesh: {}\n", assetMesh.name);
                for (auto &primitive : assetMesh.primitives) {
                    // fmt::println(stderr, "Primitive");

                    getAttributes(asset, primitive, mesh);

                    if (primitive.materialIndex.has_value()) {
                        // fmt::println(
                        //     stderr,
                        //     "MaterialIndex: {}",
                        //     primitive.materialIndex.value());
                        getMaterial(asset, primitive, scene, mesh, directory);
                    }
                }
            }
        });

    // add a final "default" camera
    scene.cameras.emplace_back();

    return scene;
}
