#pragma once

#include "vulkan_device.h"

// std
#include <string>
#include <vector>

namespace mvr
{

struct PipelineConfigInfo {
	VkViewport                             viewport;
	VkRect2D                               scissor;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo   multisampleInfo;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo  depthStencilInfo;
	VkPipelineLayout                       pipelineLayout = VK_NULL_HANDLE;
	VkRenderPass                           renderPass     = VK_NULL_HANDLE;
	uint32_t                               subpass        = 0;
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
	static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

  private:
	static std::vector<char> readFile(const std::string &filePath);
	vk::raii::ShaderModule   createShaderModule(const std::vector<char> &code);
	void                     createGraphicsPipeline(const std::string        &vertFilePath,
	                                                const std::string        &fragFilePath,
	                                                const PipelineConfigInfo &configInfo);

	VulkanDevice &device;
	VkPipeline    graphicsPipeline;
};

}        // namespace mvr