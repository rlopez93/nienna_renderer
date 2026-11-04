#pragma once

#include <filesystem>

#include "RenderAsset.hpp"

auto getAsset(std::filesystem::path const& gltfPath) -> RenderAsset;
