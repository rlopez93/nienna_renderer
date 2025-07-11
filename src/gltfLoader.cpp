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
    auto image_data        = std::vector<unsigned char>{};
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

    fastgltf::iterateSceneNodes(
        asset,
        0,
        fastgltf::math::fmat4x4{},
        [&](fastgltf::Node &node, fastgltf::math::fmat4x4 matrix) {
            if (hasMesh && hasCamera) {
                return;
            }
            if (node.cameraIndex.has_value()) {

                fastgltf::math::fvec3 _scale;
                fastgltf::math::fquat _rotation;
                fastgltf::math::fvec3 _translation;

                fastgltf::math::decomposeTransformMatrix(
                    matrix,
                    _scale,
                    _rotation,
                    _translation);

                glm::quat rotation    = convQ(_rotation);
                glm::vec3 translation = convV(_translation);

                auto &trs = std::get<fastgltf::TRS>(node.transform);

                viewMatrix = glm::inverse(
                    glm::translate(glm::mat4(1.0f), translation)
                    * glm::mat4_cast(rotation));

                // viewMatrix = fastgltf::math::invert(
                //     fastgltf::math::translate(
                //         fastgltf::math::rotate(fastgltf::math::fmat4x4{}, rotation),
                //         translation));

                fmt::print(
                    stderr,
                    "viewMatrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} "
                    "{}\n",
                    viewMatrix[0][0],
                    viewMatrix[0][1],
                    viewMatrix[0][2],
                    viewMatrix[0][3],
                    viewMatrix[1][0],
                    viewMatrix[1][1],
                    viewMatrix[1][2],
                    viewMatrix[1][3],
                    viewMatrix[2][0],
                    viewMatrix[2][1],
                    viewMatrix[2][2],
                    viewMatrix[2][3],
                    viewMatrix[3][0],
                    viewMatrix[3][1],
                    viewMatrix[3][2],
                    viewMatrix[3][3]);

                auto &camera = asset.cameras[node.cameraIndex.value()];
                if (std::holds_alternative<fastgltf::Camera::Perspective>(
                        camera.camera)) {
                    auto cameraProjection =
                        std::get<fastgltf::Camera::Perspective>(camera.camera);

                    if (cameraProjection.zfar.has_value()) {
                        fmt::print(stderr, "finite perspective projection\n");
                        projectionMatrix = glm::perspectiveRH_ZO(
                            cameraProjection.yfov,
                            cameraProjection.aspectRatio.value_or(1.0f),
                            cameraProjection.znear,
                            cameraProjection.zfar.value());
                    } else {
                        fmt::print(stderr, "infinite perspective projection\n");
                        projectionMatrix = glm::infinitePerspectiveRH_ZO(
                            cameraProjection.yfov,
                            cameraProjection.aspectRatio.value_or(1.0f),
                            cameraProjection.znear);
                    }

                    hasCamera = true;
                }

                fmt::print(
                    stderr,
                    "matrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n",
                    matrix[0][0],
                    matrix[0][1],
                    matrix[0][2],
                    matrix[0][3],
                    matrix[1][0],
                    matrix[1][1],
                    matrix[1][2],
                    matrix[1][3],
                    matrix[2][0],
                    matrix[2][1],
                    matrix[2][2],
                    matrix[2][3],
                    matrix[3][0],
                    matrix[3][1],
                    matrix[3][2],
                    matrix[3][3]);

                matrix = fastgltf::getTransformMatrix(node, matrix);
                fmt::print(
                    stderr,
                    "local matrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} "
                    "{}\n",
                    matrix[0][0],
                    matrix[0][1],
                    matrix[0][2],
                    matrix[0][3],
                    matrix[1][0],
                    matrix[1][1],
                    matrix[1][2],
                    matrix[1][3],
                    matrix[2][0],
                    matrix[2][1],
                    matrix[2][2],
                    matrix[2][3],
                    matrix[3][0],
                    matrix[3][1],
                    matrix[3][2],
                    matrix[3][3]);
            }

            else if (node.meshIndex.has_value()) {
                auto &mesh = asset.meshes[node.meshIndex.value()];
                fmt::print(stderr, "Mesh is: <{}>\n", mesh.name);

                modelMatrix = convM(matrix);

                fmt::print(
                    stderr,
                    "matrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n",
                    matrix[0][0],
                    matrix[0][1],
                    matrix[0][2],
                    matrix[0][3],
                    matrix[1][0],
                    matrix[1][1],
                    matrix[1][2],
                    matrix[1][3],
                    matrix[2][0],
                    matrix[2][1],
                    matrix[2][2],
                    matrix[2][3],
                    matrix[3][0],
                    matrix[3][1],
                    matrix[3][2],
                    matrix[3][3]);

                for (auto primitiveIt = mesh.primitives.begin();
                     primitiveIt != mesh.primitives.end();
                     ++primitiveIt) {

                    if (primitiveIt->indicesAccessor.has_value()) {
                        auto &indexAccessor =
                            asset.accessors[primitiveIt->indicesAccessor.value()];
                        indices.resize(indexAccessor.count);

                        fastgltf::iterateAccessorWithIndex<std::uint16_t>(
                            asset,
                            indexAccessor,
                            [&](std::uint16_t index, std::size_t idx) {
                                indices[idx] = index;
                            });
                    }

                    auto positionIt = primitiveIt->findAttribute("POSITION");
                    auto normalIt   = primitiveIt->findAttribute("NORMAL");
                    auto texCoordIt = primitiveIt->findAttribute("TEXCOORD_0");

                    assert(positionIt != primitiveIt->attributes.end());
                    assert(normalIt != primitiveIt->attributes.end());
                    assert(texCoordIt != primitiveIt->attributes.end());

                    auto &positionAccessor = asset.accessors[positionIt->accessorIndex];
                    auto &normalAccessor   = asset.accessors[normalIt->accessorIndex];
                    auto &texCoordAccessor = asset.accessors[texCoordIt->accessorIndex];

                    vertices.resize(positionAccessor.count);

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        asset,
                        positionAccessor,
                        [&](glm::vec3 position, std::size_t idx) {
                            vertices[idx].position = position;
                        });

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        asset,
                        normalAccessor,
                        [&](glm::vec3 normal, std::size_t idx) {
                            vertices[idx].normal = normal;
                        });

                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                        asset,
                        texCoordAccessor,
                        [&](glm::vec2 texCoord, std::size_t idx) {
                            vertices[idx].texCoord = texCoord;
                        });

                    assert(primitiveIt->materialIndex.has_value());
                    fastgltf::Material &material =
                        asset.materials[primitiveIt->materialIndex.value()];

                    if (!material.pbrData.baseColorTexture.has_value()) {
                        continue;
                    }
                    fastgltf::TextureInfo &textureInfo =
                        material.pbrData.baseColorTexture.value();

                    assert(textureInfo.texCoordIndex == 0);
                    fastgltf::Texture &texture =
                        asset.textures[textureInfo.textureIndex];

                    assert(texture.imageIndex.has_value());
                    fastgltf::Image &image = asset.images[texture.imageIndex.value()];

                    fastgltf::URI &imageURI =
                        std::get<fastgltf::sources::URI>(image.data).uri;

                    {
                        auto path = directory / imageURI.fspath();
                        fmt::print(stderr, "\"{}\"\n", path.c_str());
                        int  x, y, n;
                        auto stbi_data = stbi_load(path.c_str(), &x, &y, &n, 4);

                        fmt::print(stderr, "channels in file: {}\n", n);
                        assert(stbi_data);

                        image_data.assign(stbi_data, stbi_data + (x * y * 4));
                        // assert(strcmp(image_data.data(), stbi_data) == 0);
                        imageExtent.setWidth(x).setHeight(y);

                        stbi_image_free(stbi_data);
                    }

                    assert(texture.samplerIndex.has_value());
                    fastgltf::Sampler &textureSampler =
                        asset.samplers[texture.samplerIndex.value()];

                    // TODO make vk::SamplerCreateInfo from fastgltf::Sampler

                    hasMesh = true;
                }
            }
        });

    if (!hasCamera) {
        viewMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f));
        projectionMatrix = glm::perspectiveRH_ZO(0.66f, 1.5f, 1.0f, 1000.0f);
    }

    return {
        indices,
        vertices,
        image_data,
        imageExtent,
        modelMatrix,
        viewMatrix,
        projectionMatrix,
        samplerCreateInfo};
}
