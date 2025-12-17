#pragma once

#include <fastgltf/core.hpp>

#include <filesystem>

#include "Scene.hpp"

auto getGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset;

auto getSceneData(
    const fastgltf::Asset       &asset,
    const std::filesystem::path &directory) -> Scene;
