#pragma once

#include "ShaderInterfaceTypes.hpp"

struct Material;

[[nodiscard]]
auto packMaterialData(const Material &material) -> MaterialData;
