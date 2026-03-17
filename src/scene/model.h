#pragma once

#include "core/vulkan_device.h"
#include <vulkan/vulkan_core.h>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace mvr
{

class Model
{
  public:
	struct Vertex {
		glm::vec2 position;
		glm::vec3 color;

		static std::vector<VkVertexInputBindingDescription>   getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	};

	Model(VulkanDevice &device, const std::vector<Vertex> &vertices);
	~Model();

	Model(const Model &)            = delete;
	Model &operator=(const Model &) = delete;

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

  private:
	VulkanDevice  &device;
	VkBuffer       vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	uint32_t       vertexCount;

	void createVertexBuffers(const std::vector<Vertex> &vertices);
};

}        // namespace mvr