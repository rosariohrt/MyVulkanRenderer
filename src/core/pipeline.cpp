#include "pipeline.h"

#include <scene/model.h>

// std
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace mvr
{

Pipeline::Pipeline(VulkanDevice             &device,
                   const std::string        &vertFilePath,
                   const std::string        &fragFilePath,
                   const PipelineConfigInfo &configInfo) : device{device}
{
	createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
}

Pipeline::~Pipeline()
{
}

void Pipeline::bind(const vk::raii::CommandBuffer &commandBuffer)
{
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
}

PipelineConfigInfo Pipeline::defaultPipelineConfigInfo(vk::Format format)
{
	PipelineConfigInfo configInfo{};

	// inputAssembly
	configInfo.inputAssembly = {
	    .topology               = vk::PrimitiveTopology::eTriangleList,
	    .primitiveRestartEnable = vk::False,
	};
	// viewport
	configInfo.viewport = vk::PipelineViewportStateCreateInfo{
	    // TODO: abbreviate this struct declaration
	    .viewportCount = 1,
	    .pViewports    = nullptr,
	    .scissorCount  = 1,
	    .pScissors     = nullptr,
	};
	// rasterization
	configInfo.rasterization = {
	    .depthClampEnable        = vk::False,
	    .rasterizerDiscardEnable = vk::False,
	    .polygonMode             = vk::PolygonMode::eFill,
	    .cullMode                = vk::CullModeFlagBits::eBack,
	    .frontFace               = vk::FrontFace::eCounterClockwise,
	    .depthBiasEnable         = vk::False,
	    .depthBiasConstantFactor = 0.0f,        // optional
	    .depthBiasClamp          = 0.0f,        // optional
	    .depthBiasSlopeFactor    = 0.0f,        // optional
	    .lineWidth               = 1.0f,
	};
	// multisample
	configInfo.multisample = {
	    .rasterizationSamples  = vk::SampleCountFlagBits::e1,
	    .sampleShadingEnable   = vk::False,
	    .minSampleShading      = 1.0f,             // optional
	    .pSampleMask           = nullptr,          // optional
	    .alphaToCoverageEnable = vk::False,        // optional
	    .alphaToOneEnable      = vk::False,        // optional
	};
	// depthStencilInfo
	configInfo.depthStencil = {
	    .depthTestEnable       = vk::True,
	    .depthWriteEnable      = vk::True,
	    .depthCompareOp        = vk::CompareOp::eLess,
	    .depthBoundsTestEnable = vk::False,
	    .stencilTestEnable     = vk::False,
	    .front                 = {},          // optional
	    .back                  = {},          // optional
	    .minDepthBounds        = 0.0f,        // optional
	    .maxDepthBounds        = 1.0f,        // optional
	};
	// colorBlendAttachment
	configInfo.colorBlendAttachment = {
	    .blendEnable         = vk::False,
	    .srcColorBlendFactor = vk::BlendFactor::eOne,         // optional
	    .dstColorBlendFactor = vk::BlendFactor::eZero,        // optional
	    .colorBlendOp        = vk::BlendOp::eAdd,             // optional
	    .srcAlphaBlendFactor = vk::BlendFactor::eOne,         // optional
	    .dstAlphaBlendFactor = vk::BlendFactor::eZero,        // optional
	    .alphaBlendOp        = vk::BlendOp::eAdd,             // optional
	    .colorWriteMask =
	        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
	};
	// blendConstants
	std::array blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
	// colorBlend
	configInfo.colorBlend = {
	    .logicOpEnable   = vk::False,
	    .logicOp         = vk::LogicOp::eCopy,
	    .attachmentCount = 1,
	    .pAttachments    = &configInfo.colorBlendAttachment,
	    .blendConstants  = blendConstants,
	};
	// rendering
	configInfo.rendering = {
	    .colorAttachmentCount    = 1,
	    .pColorAttachmentFormats = &format,
	};

	// dynamicState
	configInfo.dynamicStates = {
	    vk::DynamicState::eViewport,
	    vk::DynamicState::eScissor,
	};
	configInfo.dynamicStateInfo = {
	    .dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStates.size()),
	    .pDynamicStates    = configInfo.dynamicStates.data(),
	};

	return configInfo;
}

std::vector<char> Pipeline::readFile(const std::string &filePath)
{
	std::ifstream file{filePath, std::ios::ate | std::ios::binary};

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filePath);
	}

	size_t            fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

void Pipeline::createGraphicsPipeline(const std::string        &vertFilePath,
                                      const std::string        &fragFilePath,
                                      const PipelineConfigInfo &configInfo)
{
	assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create pipeline with no layout provided");

	// create shader modules and shader stages
	vk::raii::ShaderModule vertexShaderModule   = createShaderModule(readFile(vertFilePath));
	vk::raii::ShaderModule fragmentShaderModule = createShaderModule(readFile(fragFilePath));

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {
	    .stage  = vk::ShaderStageFlagBits::eVertex,
	    .module = vertexShaderModule,
	    .pName  = "main",
	};
	vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {
	    .stage  = vk::ShaderStageFlagBits::eFragment,
	    .module = fragmentShaderModule,
	    .pName  = "main",
	};

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// create vertex input state
	auto                                   bindingDescriptions   = Model::Vertex::getBindingDescriptions();
	auto                                   attributeDescriptions = Model::Vertex::getAttributeDescriptions();
	vk::PipelineVertexInputStateCreateInfo vertexInput           = {
	              .vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size()),
	              .pVertexBindingDescriptions      = bindingDescriptions.data(),
	              .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
	              .pVertexAttributeDescriptions    = attributeDescriptions.data(),
    };

	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineInfoChain = {
	    {
	        .stageCount          = 2,
	        .pStages             = shaderStages,
	        .pVertexInputState   = &vertexInput,
	        .pInputAssemblyState = &configInfo.inputAssembly,
	        .pViewportState      = &configInfo.viewport,
	        .pRasterizationState = &configInfo.rasterization,
	        .pMultisampleState   = &configInfo.multisample,
	        .pDepthStencilState  = &configInfo.depthStencil,
	        .pColorBlendState    = &configInfo.colorBlend,
	        .pDynamicState       = &configInfo.dynamicStateInfo,
	        .layout              = configInfo.pipelineLayout,
	    },
	    {
	        .colorAttachmentCount    = configInfo.rendering.colorAttachmentCount,
	        .pColorAttachmentFormats = configInfo.rendering.pColorAttachmentFormats,
	    }};

	graphicsPipeline = vk::raii::Pipeline(device.device(), nullptr, pipelineInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

vk::raii::ShaderModule Pipeline::createShaderModule(const std::vector<char> &code)
{
	vk::ShaderModuleCreateInfo createInfo = {
	    .codeSize = code.size() * sizeof(char),
	    .pCode    = reinterpret_cast<const uint32_t *>(code.data()),
	};

	vk::raii::ShaderModule shaderModule(device.device(), createInfo);

	return shaderModule;
}

}        // namespace mvr