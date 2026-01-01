#include "GltfLoader.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>
#include <fmt/format.h>

#include "Material.hpp"

namespace
{

template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

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

auto parseGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset;

auto extractAsset(
    const fastgltf::Asset       &gltf,
    const std::filesystem::path &baseDir,
    std::uint32_t                initialScene) -> Asset;

auto postProcessAsset(Asset &asset) -> void;

auto parseGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset
{
    constexpr auto supportedExtensions = fastgltf::Extensions::KHR_texture_transform;

    fastgltf::Parser parser(supportedExtensions);

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                               | fastgltf::Options::LoadExternalBuffers
                               | fastgltf::Options::LoadExternalImages
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

auto extractAsset(
    const fastgltf::Asset       &gltfAsset,
    const std::filesystem::path &baseDir,
    std::uint32_t                initialScene) -> Asset
{
    Asset out{};

    out.activeScene = gltfAsset.defaultScene.value_or(initialScene);

    // TODO:
    // - default textures using kDefaultTextureCount
    // - default material at index 0
    // - cameras (visitor on gltf camera variant)
    // - nodes (children, TRS, mesh, camera)
    // - scenes (root nodes) + activeScene selection
    // - images/textures/samplers (RGBA8 + samplerInfo)
    // - materials (core fields + alpha + cull)
    // - meshes/primitives -> MeshAsset/Submesh decode

    return out;
}

auto postProcessAsset(Asset &asset) -> void
{
    (void)asset;

    // TODO:
    // - generate flat normals for missing NORMAL
    // - later: MikkTSpace tangents when needed
}

} // namespace

auto getAsset(
    const std::filesystem::path &gltfPath,
    std::uint32_t                initialScene) -> Asset
{
    auto gltf = parseGltfAsset(gltfPath);

    auto asset = extractAsset(gltf, gltfPath.parent_path(), initialScene);

    postProcessAsset(asset);
    return asset;
}
