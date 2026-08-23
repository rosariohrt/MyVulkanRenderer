#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "core/vulkan_device.h"

// libs
#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace mvr
{

class ImGuiRenderer
{
  public:
	ImGuiRenderer(VulkanDevice &device, vk::Extent2D extent);

	ImGuiRenderer(const ImGuiRenderer &)            = delete;
	ImGuiRenderer &operator=(const ImGuiRenderer &) = delete;

	void setStyle(uint32_t index);

  private:
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstantBlock;

	VulkanDevice &device;

	ImGuiStyle vulkanStyle;

	vk::raii::Sampler      fontSampler        = nullptr;
	vk::raii::Buffer       vertexBuffer       = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	uint32_t               vertexCount        = 0;
	vk::raii::Buffer       indexBuffer        = nullptr;
	vk::raii::DeviceMemory indexBufferMemory  = nullptr;
	uint32_t               indexCount         = 0;
	bool                   needsUpdateBuffers = false;
	vk::raii::Image        fontImage          = nullptr;
	vk::raii::ImageView    fontImageView      = nullptr;

	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
	vk::raii::DescriptorPool      descriptorPool      = nullptr;
	vk::raii::DescriptorSet       descriptorSet       = nullptr;
	vk::raii::PipelineCache       pipelineCache       = nullptr;
	vk::raii::PipelineLayout      pipelineLayout      = nullptr;
	vk::raii::Pipeline            pipeline            = nullptr;

	vk::PipelineRenderingCreateInfo renderingInfo{};
	vk::Format                      colorFormat = vk::Format::eB8G8R8A8Unorm;

	void createBuffers();
	void createImGuiContext(vk::Extent2D extent);
	void createTextureSampler();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSet();
};

}        // namespace mvr
