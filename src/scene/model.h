#pragma once

#include "core/vulkan_device.h"

// libs
#include <glm/glm.hpp>

// std
#include <vector>

namespace mvr
{

class Model
{
  public:
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoord;

		static std::vector<vk::VertexInputBindingDescription>
		    getBindingDescriptions()
		{
			return {
			    {
			        .binding   = 0,
			        .stride    = sizeof(Vertex),
			        .inputRate = vk::VertexInputRate::eVertex,
			    },
			};
		}
		static std::array<vk::VertexInputAttributeDescription, 3>
		    getAttributeDescriptions()
		{
			return {{
			    {
			        .location = 0,
			        .binding  = 0,
			        .format   = vk::Format::eR32G32B32Sfloat,
			        .offset   = offsetof(Vertex, pos),
			    },
			    {
			        .location = 1,
			        .binding  = 0,
			        .format   = vk::Format::eR32G32B32Sfloat,
			        .offset   = offsetof(Vertex, normal),
			    },
			    {
			        .location = 2,
			        .binding  = 0,
			        .format   = vk::Format::eR32G32Sfloat,
			        .offset   = offsetof(Vertex, texCoord),
			    },
			}};
		}
	};

	Model(VulkanDevice &device, const std::string &path);
	Model(const Model &)            = delete;
	Model &operator=(const Model &) = delete;

	void bind(vk::CommandBuffer commandBuffer);
	void draw(vk::CommandBuffer commandBuffer);

	vk::DescriptorBufferInfo getDescriptorBufferInfo(size_t index);

	// Getters
	vk::raii::Buffer &getUniformBuffers(size_t index)
	{
		return uniformBuffers[index];
	}
	void *getUniformBuffersMapped(uint32_t frameIndex)
	{
		return uniformBuffersMapped[frameIndex];
	}

  private:
	VulkanDevice                       &device;
	vk::raii::Buffer                    vertexBuffer       = nullptr;
	vk::raii::DeviceMemory              vertexBufferMemory = nullptr;
	uint32_t                            vertexCount;
	vk::raii::Buffer                    indexBuffer       = nullptr;
	vk::raii::DeviceMemory              indexBufferMemory = nullptr;
	uint32_t                            indexCount;
	std::vector<vk::raii::Buffer>       uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
	std::vector<void *>                 uniformBuffersMapped;

	void loadGltfModel(std::vector<Vertex>   &vertices,
	                   std::vector<uint32_t> &indices,
	                   const std::string     &path);
	void createVertexBuffer(const std::vector<Vertex> &vertices);
	void createIndexBuffer(const std::vector<uint32_t> &indices);
	void createUniformBuffers();
};

}        // namespace mvr
