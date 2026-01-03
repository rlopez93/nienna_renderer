#pragma once

#include <filesystem>

#include "Asset.hpp"

auto getAsset(const std::filesystem::path &gltfPath) -> Asset;
