#include "MaterialPacking.hpp"
#include "Material.hpp"

namespace
{

[[nodiscard]]
auto packTextureTransform2DData(const UVTransform2D &transform)
    -> TextureTransform2DData
{
    TextureTransform2DData data{};

    data.offset   = transform.offset;
    data.scale    = transform.scale;
    data.rotation = transform.rotation;
    data.enabled  = (transform.enabled) ? 1u : 0u;

    return data;
}

[[nodiscard]]
auto packTextureRefData(const TextureRef &textureRef) -> TextureRefData
{
    TextureRefData data{};

    data.textureIndex = textureRef.textureIndex;
    data.texCoord     = textureRef.texCoord;
    data.transform    = packTextureTransform2DData(textureRef.uvTransform);

    return data;
}

} // namespace

auto packMaterialData(const Material &material) -> MaterialData
{
    MaterialData packed{};

    packed.baseColorFactor   = material.core.baseColorFactor;
    packed.emissiveFactor    = material.core.emissiveFactor;
    packed.metallicFactor    = material.core.metallicFactor;
    packed.roughnessFactor   = material.core.roughnessFactor;
    packed.normalScale       = material.core.normalScale;
    packed.occlusionStrength = material.core.occlusionStrength;
    packed.alphaCutoff       = material.core.alphaCutoff;

    packed.flags = 0u;

    if (material.core.alphaMaskEnable) {
        packed.flags |= kMaterialFlagAlphaMask;
    }

    if (material.core.alphaBlendEnable) {
        packed.flags |= kMaterialFlagAlphaBlend;
    }

    if (material.core.doubleSided) {
        packed.flags |= kMaterialFlagDoubleSided;
    }

    if (material.unlit.enabled) {
        packed.flags |= kMaterialFlagUnlit;
    }

    packed.ior = material.ior.ior;

    if (material.ior.enabled) {
        packed.flags |= kMaterialFlagIor;
    }

    packed.specularFactor      = material.specular.specularFactor;
    packed.specularColorFactor = material.specular.specularColorFactor;

    if (material.specular.enabled) {
        packed.flags |= kMaterialFlagSpecular;
    }

    packed.clearcoatFactor          = material.clearcoat.clearcoatFactor;
    packed.clearcoatRoughnessFactor = material.clearcoat.clearcoatRoughnessFactor;

    if (material.clearcoat.enabled) {
        packed.flags |= kMaterialFlagClearcoat;
    }

    packed.baseColorTexture = packTextureRefData(material.core.baseColorTexture);
    packed.metallicRoughnessTexture =
        packTextureRefData(material.core.metallicRoughnessTexture);
    packed.normalTexture    = packTextureRefData(material.core.normalTexture);
    packed.occlusionTexture = packTextureRefData(material.core.occlusionTexture);
    packed.emissiveTexture  = packTextureRefData(material.core.emissiveTexture);
    packed.specularTexture  = packTextureRefData(material.specular.specularTexture);
    packed.specularColorTexture =
        packTextureRefData(material.specular.specularColorTexture);
    packed.clearcoatTexture = packTextureRefData(material.clearcoat.clearcoatTexture);
    packed.clearcoatRoughnessTexture =
        packTextureRefData(material.clearcoat.clearcoatRoughnessTexture);
    packed.clearcoatNormalTexture =
        packTextureRefData(material.clearcoat.clearcoatNormalTexture);

    return packed;
}
