#pragma once

#include "core/vulkan_device.h"

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

		static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions()
		{
			return {
			    {
			        .binding   = 0,
			        .stride    = sizeof(Vertex),
			        .inputRate = vk::VertexInputRate::eVertex,
			    },
			};
		}
		static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
		{
			return {
			    {
			        .location = 0,
			        .binding  = 0,
			        .format   = vk::Format::eR32G32Sfloat,
			        .offset   = offsetof(Vertex, position),
			    },
			    {
			        .location = 1,
			        .binding  = 0,
			        .format   = vk::Format::eR32G32B32Sfloat,
			        .offset   = offsetof(Vertex, color),
			    },
			};
		}
	};

	Model(VulkanDevice &device, const std::vector<Vertex> &vertices);
	Model(const Model &)            = delete;
	Model &operator=(const Model &) = delete;

	void bind(vk::CommandBuffer commandBuffer);
	void draw(vk::CommandBuffer commandBuffer);

  private:
	VulkanDevice          &device;
	vk::raii::Buffer       vertexBuffer       = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	uint32_t               vertexCount;

	void createVertexBuffers(const std::vector<Vertex> &vertices);
};

}        // namespace mvr