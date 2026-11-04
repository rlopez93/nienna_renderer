#include "Pipeline.hpp"
#include "Geometry.hpp"
#include "Shader.hpp"

auto createPipeline(
    Device&                      device,
    std::filesystem::path const& shaderPath,
    vk::Format                   imageFormat,
    vk::Format                   depthFormat,
    vk::raii::PipelineLayout&    pipelineLayout) -> vk::raii::Pipeline
{
    auto shaderModule = createShaderModule(device.handle, shaderPath);

    auto const shaderStages = std::array{
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eVertex,
            shaderModule,
            "vertexMain",
            {},
        },
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            shaderModule,
            "fragmentMain",
            {},
        },
    };

    auto const vertexBindingDescriptions = std::array{vk::VertexInputBindingDescription{
        0u,
        static_cast<std::uint32_t>(sizeof(MeshVertex)),
        vk::VertexInputRate::eVertex,
    }};

    auto const vertexAttributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{
            0u,
            0u,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, position)),
        },
        vk::VertexInputAttributeDescription{
            1u,
            0u,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, normal)),
        },
        vk::VertexInputAttributeDescription{
            2u,
            0u,
            vk::Format::eR32G32B32A32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, tangent)),
        },
        vk::VertexInputAttributeDescription{
            3u,
            0u,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, uv0)),
        },
        vk::VertexInputAttributeDescription{
            4u,
            0u,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, uv1)),
        },
        vk::VertexInputAttributeDescription{
            5u,
            0u,
            vk::Format::eR32G32B32A32Sfloat,
            static_cast<std::uint32_t>(offsetof(MeshVertex, color)),
        },
    };

    auto const vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        {},
        vertexBindingDescriptions,
        vertexAttributeDescriptions,
    };

    auto const inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        {},
        vk::PrimitiveTopology::eTriangleList,
        false,
    };

    auto const pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{};

    auto const pipelineRasterizationStateCreateInfo =
        vk::PipelineRasterizationStateCreateInfo{
            {},
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
    };

    auto const pipelineMultisampleStateCreateInfo =
        vk::PipelineMultisampleStateCreateInfo{};

    auto const pipelineDepthStencilStateCreateInfo =
        vk::PipelineDepthStencilStateCreateInfo{
            {},
            true,
            true,
            vk::CompareOp::eLessOrEqual,
    };

    auto const colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{
        false,
        {},
        {},
        {},
        {},
        {},
        {},
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    auto const pipelineColorBlendStateCreateInfo =
        vk::PipelineColorBlendStateCreateInfo{
            {},
            false,
            vk::LogicOp::eCopy,
            1u,
            &colorBlendAttachmentState,
    };

    auto const dynamicStates = std::array{
        vk::DynamicState::eViewportWithCount,
        vk::DynamicState::eScissorWithCount,
        vk::DynamicState::ePolygonModeEXT,
    };

    auto const pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{
        {},
        dynamicStates,
    };

    auto pipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{
        {},
        imageFormat,
        depthFormat,
    };

    auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo{
        {},
        shaderStages,
        &vertexInputStateCreateInfo,
        &inputAssemblyCreateInfo,
        {},
        &pipelineViewportStateCreateInfo,
        &pipelineRasterizationStateCreateInfo,
        &pipelineMultisampleStateCreateInfo,
        &pipelineDepthStencilStateCreateInfo,
        &pipelineColorBlendStateCreateInfo,
        &pipelineDynamicStateCreateInfo,
        *pipelineLayout,
        {},
        {},
        {},
        {},
        &pipelineRenderingCreateInfo,
    };

    return vk::raii::Pipeline{
        device.handle,
        nullptr,
        graphicsPipelineCreateInfo,
    };
}
