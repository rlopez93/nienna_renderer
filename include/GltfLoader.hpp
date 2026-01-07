#pragma once

#include <filesystem>

#include "RenderAsset.hpp"

auto getAsset(const std::filesystem::path &gltfPath) -> RenderAsset;
