#include "Pipeline.hpp"
#include "Geometry.hpp"
#include "Shader.hpp"

auto createPipeline(
    Device                      &device,
    const std::filesystem::path &shaderPath,
    vk::Format                   imageFormat,
    vk::Format                   depthFormat,
    vk::raii::PipelineLayout    &pipelineLayout) -> vk::raii::Pipeline
{
    auto shaderModule = createShaderModule(device.handle, shaderPath);

    const auto shaderStages = std::array{
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

    const auto vertexBindingDescriptions = std::array{vk::VertexInputBindingDescription{
        0u,
        static_cast<std::uint32_t>(sizeof(Vertex)),
        vk::VertexInputRate::eVertex,
    }};

    const auto vertexAttributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{
            0u,
            0u,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(Vertex, position)),
        },
        vk::VertexInputAttributeDescription{
            1u,
            0u,
            vk::Format::eR32G32B32Sfloat,
            static_cast<std::uint32_t>(offsetof(Vertex, normal)),
        },
        vk::VertexInputAttributeDescription{
            2u,
            0u,
            vk::Format::eR32G32B32A32Sfloat,
            static_cast<std::uint32_t>(offsetof(Vertex, tangent)),
        },
        vk::VertexInputAttributeDescription{
            3u,
            0u,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(Vertex, uv0)),
        },
        vk::VertexInputAttributeDescription{
            4u,
            0u,
            vk::Format::eR32G32Sfloat,
            static_cast<std::uint32_t>(offsetof(Vertex, uv1)),
        },
        vk::VertexInputAttributeDescription{
            5u,
            0u,
            vk::Format::eR32G32B32A32Sfloat,
            static_cast<std::uint32_t>(offsetof(Vertex, color)),
        },
    };

    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        {},
        vertexBindingDescriptions,
        vertexAttributeDescriptions,
    };

    const auto inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        {},
        vk::PrimitiveTopology::eTriangleList,
        false,
    };

    const auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{};

    const auto pipelineRasterizationStateCreateInfo =
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

    const auto pipelineMultisampleStateCreateInfo =
        vk::PipelineMultisampleStateCreateInfo{};

    const auto pipelineDepthStencilStateCreateInfo =
        vk::PipelineDepthStencilStateCreateInfo{
            {},
            true,
            true,
            vk::CompareOp::eLessOrEqual,
        };

    const auto colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{
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

    const auto pipelineColorBlendStateCreateInfo =
        vk::PipelineColorBlendStateCreateInfo{
            {},
            false,
            vk::LogicOp::eCopy,
            1u,
            &colorBlendAttachmentState,
        };

    const auto dynamicStates = std::array{
        vk::DynamicState::eViewportWithCount,
        vk::DynamicState::eScissorWithCount,
        vk::DynamicState::ePolygonModeEXT,
    };

    const auto pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{
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
