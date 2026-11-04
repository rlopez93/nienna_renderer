#pragma once

#include "ShaderInterfaceTypes.hpp"

struct Material;

[[nodiscard]]
auto packMaterialData(Material const& material) -> MaterialData;
