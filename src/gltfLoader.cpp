#include "gltfLoader.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fmt/base.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>

#include <stb_image.h>

#include <glm/common.hpp>

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
        fmt::print(
            stderr,
            "Failed to open glTF file: {}\n",
            fastgltf::getErrorMessage(gltfFile.error()));
        throw std::exception{};
    }

    auto asset = parser.loadGltf(gltfFile.get(), gltfPath.parent_path(), gltfOptions);

    if (fastgltf::getErrorName(asset.error()) != "None") {
        fmt::print(
            stderr,
            "Failed to open glTF file: {}\n",
            fastgltf::getErrorMessage(gltfFile.error()));
    }

    return std::move(asset.get());
}

auto convM(const fastgltf::math::fmat4x4 &M) -> glm::mat4
{
    glm::mat4 N;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            N[i][j] = M[i][j];
        }
    }
    return N;
}

auto convQ(const fastgltf::math::fquat &Q) -> glm::quat
{
    return {Q.w(), Q.x(), Q.y(), Q.z()};
}

auto convV(const fastgltf::math::fvec3 &V) -> glm::vec3
{
    return {V.x(), V.y(), V.z()};
}

// helper type for the visitor
template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

void getMaterial(

    const fastgltf::Asset       &asset,
    const fastgltf::Material    &material,
    vk::Extent2D                &imageExtent,
    std::vector<unsigned char>  &imageData,
    const std::filesystem::path &directory

)
{
    if (material.pbrData.baseColorTexture.has_value()) {

        auto &textureInfo = material.pbrData.baseColorTexture.value();

        // assert(textureInfo.texCoordIndex == 0);
        auto &texture = asset.textures[textureInfo.textureIndex];

        assert(texture.imageIndex.has_value());
        auto &image = asset.images[texture.imageIndex.value()];

        /**
         * Represents the data source of a buffer or image.
         * These could be a buffer view, a file path (including
         * offsets), a StaticVector (if
         * #Options::LoadExternalBuffers or
         * #Options::LoadGLBBuffers was specified), or the ID of
         * a custom buffer.
         *
         * @note As a user, you should never encounter this
         * variant holding the std::monostate, as that would be
         * an ill-formed glTF, which fastgltf already checks for
         * while parsing.
         *
         * @note For buffers, this variant will never hold a
         * sources::BufferView, as only images are able to
         * reference buffer views as a source.
         */
        // FASTGLTF_EXPORT using DataSource =
        // std::variant<std::monostate, sources::BufferView,
        // sources::URI, sources::Array, sources::Vector,
        // sources::CustomBuffer, sources::ByteView,
        // sources::Fallback>;

        namespace sources = fastgltf::sources;

        const auto visitor = overloads{
            [](std::monostate) { throw std::logic_error{"std::monostate"}; },
            [&directory, &imageData, &imageExtent](const sources::URI &imageURI) {
                const auto path = directory / imageURI.uri.fspath();
                fmt::println(stderr, "\"{}\"", path.c_str());
                int  x, y, n;
                auto stbiData = stbi_load(path.c_str(), &x, &y, &n, 4);

                fmt::print(stderr, "channels in file: {}\n", n);
                assert(stbiData);

                imageData.assign(stbiData, stbiData + (x * y * 4));

                imageExtent.setWidth(x).setHeight(y);

                stbi_image_free(stbiData);
            },
            [](const sources::BufferView &view) {
                throw std::logic_error{"sources::BufferView"};
            },
            [](const sources::Array &data) {
                throw std::logic_error{"sources::Array"};
            },
            [](const sources::Vector &data) {
                throw std::logic_error{"sources::Vector"};
            },
            [](const sources::CustomBuffer &buf) {
                throw std::logic_error{"sources::CustomBuffer"};
            },
            [](const sources::ByteView &byteView) {
                throw std::logic_error{"sources::byteView"};
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

        // assert(texture.samplerIndex.has_value());
        // fastgltf::Sampler &textureSampler =
        //     asset.samplers[texture.samplerIndex.value()];

        // TODO make vk::SamplerCreateInfo from
        // fastgltf::Sampler
    }
}

auto getCamera(
    const fastgltf::Camera        &camera,
    const fastgltf::math::fmat4x4 &matrix,
    glm::mat4                     &projectionMatrix,
    glm::mat4                     &viewMatrix) -> bool
{
    fastgltf::math::fvec3 _scale;
    fastgltf::math::fquat _rotation;
    fastgltf::math::fvec3 _translation;

    fastgltf::math::decomposeTransformMatrix(matrix, _scale, _rotation, _translation);

    glm::quat rotation    = convQ(_rotation);
    glm::vec3 translation = convV(_translation);

    viewMatrix = glm::inverse(
        glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation));

    if (std::holds_alternative<fastgltf::Camera::Perspective>(camera.camera)) {
        auto cameraProjection = std::get<fastgltf::Camera::Perspective>(camera.camera);

        if (cameraProjection.zfar.has_value()) {
            fmt::print(stderr, "finite perspective projection\n");
            projectionMatrix = glm::perspectiveRH_ZO(
                cameraProjection.yfov,
                cameraProjection.aspectRatio.value_or(1.5f),
                cameraProjection.znear,
                cameraProjection.zfar.value());
        } else {
            fmt::print(stderr, "infinite perspective projection\n");
            projectionMatrix = glm::infinitePerspectiveRH_ZO(
                cameraProjection.yfov,
                cameraProjection.aspectRatio.value_or(1.5f),
                cameraProjection.znear);
        }

        return true;
    }

    return false;
}

auto getGltfAssetData(
    fastgltf::Asset             &asset,
    const std::filesystem::path &directory)
    -> std::tuple<
        std::vector<uint16_t>,
        std::vector<Vertex>,
        std::vector<unsigned char>,
        vk::Extent2D,
        glm::mat4,
        glm::mat4,
        glm::mat4,
        vk::SamplerCreateInfo>
{
    auto indices           = std::vector<uint16_t>{};
    auto vertices          = std::vector<Vertex>{};
    auto imageData         = std::vector<unsigned char>{};
    auto imageExtent       = vk::Extent2D{};
    auto modelMatrix       = glm::mat4(1.0f);
    auto viewMatrix        = glm::mat4(1.0f);
    auto projectionMatrix  = glm::mat4(1.0f);
    auto samplerCreateInfo = vk::SamplerCreateInfo{};

    // auto &scenes = asset.scenes;
    //
    // for (auto &scene : scenes) {
    //     for (auto rootIdx : scene.nodeIndices) {
    //         auto &root = asset.nodes[rootIdx];
    //     }
    // }
    //

    bool hasMesh   = false;
    bool hasCamera = false;

    fmt::print(stderr, "getGltfAssetData()\n");

    fastgltf::iterateSceneNodes(
        asset,
        0,
        fastgltf::math::fmat4x4{},
        [&](fastgltf::Node &node, fastgltf::math::fmat4x4 matrix) {
            fmt::print(stderr, "Node is <{}>\n", node.name);
            // if (hasMesh && hasCamera) {
            //     return;
            // }
            if (node.cameraIndex.has_value()) {
                fmt::print(stderr, "has cameraIndex\n");

                hasCamera = getCamera(
                    asset.cameras[node.cameraIndex.value()],
                    matrix,
                    projectionMatrix,
                    viewMatrix);
            }

            if (node.meshIndex.has_value()) {
                auto &mesh = asset.meshes[node.meshIndex.value()];
                fmt::print(stderr, "Mesh is: <{}>\n", mesh.name);

                modelMatrix = convM(matrix);

                for (auto &primitive : mesh.primitives) {

                    for (auto &attribute : primitive.attributes) {
                        fmt::println(stderr, "{}\n", attribute.name);

                        auto &accessor = asset.accessors[attribute.accessorIndex];

                        switch (accessor.type) {
                            using fastgltf::AccessorType;
                        case AccessorType::Invalid:
                            fmt::println("Invalid");
                            break;
                        case AccessorType::Vec2:
                            fmt::println("Vec2");
                            break;
                        case AccessorType::Vec3:
                            fmt::println("Vec3");
                            break;
                        case AccessorType::Vec4:
                            fmt::println("Vec4");
                            break;
                        case AccessorType::Mat2:
                            fmt::println("Mat2");
                            break;
                        case AccessorType::Mat3:
                            fmt::println("Mat3");
                            break;
                        case AccessorType::Mat4:
                            fmt::println("Mat4");
                            break;
                        case AccessorType::Scalar:
                            fmt::println("Scalar");
                            break;
                        }
                        if (attribute.name == "POSITION") {
                            auto &positionAccessor =
                                asset.accessors[attribute.accessorIndex];
                            vertices.resize(positionAccessor.count);

                            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                                asset,
                                positionAccessor,
                                [&](glm::vec3 position, std::size_t idx) {
                                    vertices[idx].position = position;
                                });
                        }

                        else if (attribute.name == "NORMAL") {
                            auto &normalAccessor =
                                asset.accessors[attribute.accessorIndex];
                            vertices.resize(normalAccessor.count);
                            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                                asset,
                                normalAccessor,
                                [&](glm::vec3 normal, std::size_t idx) {
                                    vertices[idx].normal = normal;
                                });
                        }

                        else if (attribute.name == "TANGENT") {
                            auto &tangentAccessor =
                                asset.accessors[attribute.accessorIndex];
                            vertices.resize(tangentAccessor.count);

                            fastgltf::iterateAccessorWithIndex<glm::vec4>(
                                asset,
                                tangentAccessor,
                                [&](glm::vec4 tangent, std::size_t idx) {
                                    // vertices[idx].texCoord = tangent;
                                });

                        }

                        else if (attribute.name.starts_with("TEXCOORD")) {
                            auto &texCoordAccessor =
                                asset.accessors[attribute.accessorIndex];
                            vertices.resize(texCoordAccessor.count);

                            fastgltf::iterateAccessorWithIndex<glm::vec2>(
                                asset,
                                texCoordAccessor,
                                [&](glm::vec2 texCoord, std::size_t idx) {
                                    vertices[idx].texCoord = texCoord;
                                });
                        }

                        else if (attribute.name.starts_with("COLOR")) {
                            auto &colorAccessor =
                                asset.accessors[attribute.accessorIndex];

                            vertices.resize(colorAccessor.count);
                            fastgltf::iterateAccessorWithIndex<glm::vec4>(
                                asset,
                                colorAccessor,
                                [&](glm::vec4 color, std::size_t idx) {
                                    // vertices[idx].texCoord = texCoord;
                                });
                        }
                    }

                    if (primitive.indicesAccessor.has_value()) {
                        auto &indexAccessor =
                            asset.accessors[primitive.indicesAccessor.value()];
                        indices.resize(indexAccessor.count);

                        fastgltf::iterateAccessorWithIndex<std::uint16_t>(
                            asset,
                            indexAccessor,
                            [&](std::uint16_t index, std::size_t idx) {
                                indices[idx] = index;
                            });
                    }

                    if (primitive.materialIndex.has_value()) {
                        auto &material =
                            asset.materials[primitive.materialIndex.value()];

                        getMaterial(asset, material, imageExtent, imageData, directory);
                    }
                }
            }
        });

    if (!hasCamera) {
        viewMatrix       = glm::inverse(modelMatrix);
        projectionMatrix = glm::perspectiveRH_ZO(0.66f, 1.5f, 1.0f, 1000.0f);
    }

    return {
        indices,
        vertices,
        imageData,
        imageExtent,
        modelMatrix,
        viewMatrix,
        projectionMatrix,
        samplerCreateInfo};
}
