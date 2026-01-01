#pragma once

#include <cstdint>
#include <filesystem>

#include "Asset.hpp"

auto getAsset(
    const std::filesystem::path &gltfPath,
    std::uint32_t                initialScene = 0u) -> Asset;
