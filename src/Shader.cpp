#include "Shader.hpp"

#include <fmt/base.h>

#include <cassert>
#include <fstream>

auto readShaders(const std::string &filename) -> std::vector<char>
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file.");
    }

    auto fileSize = file.tellg();
    auto buffer   = std::vector<char>(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

auto reflectShader(const std::vector<char> &code) -> VertexReflectionData
{
    auto reflection = spv_reflect::ShaderModule(code.size(), code.data());

    if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS) {
        fmt::print(
            stderr,
            "ERROR: could not process shader code (is it a valid SPIR-V bytecode?)\n");
        throw std::exception{};
    }

    auto &shaderModule = reflection.GetShaderModule();

    const auto entry_point_name = std::string{shaderModule.entry_point_name};

    fmt::print(stderr, "{}\n", entry_point_name);

    auto result = SPV_REFLECT_RESULT_NOT_READY;
    auto count  = uint32_t{0};

    auto data = VertexReflectionData{};

    result = reflection.EnumerateDescriptorBindings(&count, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    data.bindings.resize(count);

    result = reflection.EnumerateDescriptorBindings(&count, data.bindings.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    auto ToStringDescriptorType = [](SpvReflectDescriptorType value) -> std::string {
        switch (value) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
        }
        // unhandled SpvReflectDescriptorType enum value
        return "VK_DESCRIPTOR_TYPE_???";
    };

    auto ToStringResourceType = [](SpvReflectResourceType res_type) -> std::string {
        switch (res_type) {
        case SPV_REFLECT_RESOURCE_FLAG_UNDEFINED:
            return "UNDEFINED";
        case SPV_REFLECT_RESOURCE_FLAG_SAMPLER:
            return "SAMPLER";
        case SPV_REFLECT_RESOURCE_FLAG_CBV:
            return "CBV";
        case SPV_REFLECT_RESOURCE_FLAG_SRV:
            return "SRV";
        case SPV_REFLECT_RESOURCE_FLAG_UAV:
            return "UAV";
        }
        // unhandled SpvReflectResourceType enum value
        return "???";
    };

    for (auto p_binding : data.bindings) {
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        auto binding_name    = p_binding->name;
        auto descriptor_type = ToStringDescriptorType(p_binding->descriptor_type);
        auto resource_type   = ToStringResourceType(p_binding->resource_type);

        fmt::print(stderr, "{} {} {}\n", binding_name, descriptor_type, resource_type);
        if ((p_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            || (p_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            || (p_binding->descriptor_type
                == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
            || (p_binding->descriptor_type
                == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) {
            auto dim     = p_binding->image.dim;
            auto depth   = p_binding->image.depth;
            auto arrayed = p_binding->image.arrayed;
            auto ms      = p_binding->image.ms;
            auto sampled = p_binding->image.sampled;

            fmt::print(
                stderr,
                "dim {} depth {} arrayed {} ms {} sampled {}\n",
                (int)dim,
                depth,
                arrayed,
                ms,
                sampled);
        }
    }

    return data;
}
auto createShaderModule(
    vk::raii::Device            &device,
    const std::filesystem::path &path) -> vk::raii::ShaderModule
{
    auto shaderCode = readShaders(path);

    auto data = reflectShader(shaderCode);

    return vk::raii::ShaderModule{
        device,
        vk::ShaderModuleCreateInfo{
            {},
            shaderCode.size(),
            reinterpret_cast<const uint32_t *>(shaderCode.data())}};
}
