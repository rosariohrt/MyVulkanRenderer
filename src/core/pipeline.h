#pragma once

#include "vulkan_device.h"

// std
#include <string>
#include <vector>

namespace mvr
{

struct PipelineConfigInfo {
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	vk::PipelineViewportStateCreateInfo      viewport;
	vk::PipelineRasterizationStateCreateInfo rasterization;
	vk::PipelineMultisampleStateCreateInfo   multisample;
	vk::PipelineDepthStencilStateCreateInfo  depthStencil;
	vk::PipelineColorBlendAttachmentState    colorBlendAttachment;
	vk::PipelineColorBlendStateCreateInfo    colorBlend;
	vk::PipelineRenderingCreateInfo          rendering;

	vk::PipelineLayout pipelineLayout;

	std::vector<vk::DynamicState>      dynamicStates;
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
};

class Pipeline
{
  public:
	Pipeline(VulkanDevice             &device,
	         const std::string        &vertFilePath,
	         const std::string        &fragFilePath,
	         const PipelineConfigInfo &configInfo);
	~Pipeline();

	Pipeline(const Pipeline &)            = delete;
	Pipeline &operator=(const Pipeline &) = delete;

	void                      bind(VkCommandBuffer commandBuffer);
	static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height, vk::Format format);

  private:
	static std::vector<char> readFile(const std::string &filePath);
	vk::raii::ShaderModule   createShaderModule(const std::vector<char> &code);
	void                     createGraphicsPipeline(const std::string        &vertFilePath,
	                                                const std::string        &fragFilePath,
	                                                const PipelineConfigInfo &configInfo);

	VulkanDevice      &device;
	vk::raii::Pipeline graphicsPipeline = nullptr;
};

}        // namespace mvr