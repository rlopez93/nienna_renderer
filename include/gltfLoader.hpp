#pragma once

#include <fastgltf/core.hpp>

#include <filesystem>

#include "Scene.hpp"

auto getGltfAsset(std::filesystem::path const& gltfPath) -> fastgltf::Asset;

auto getSceneData(
    fastgltf::Asset const&       asset,
    std::filesystem::path const& directory) -> Scene;
