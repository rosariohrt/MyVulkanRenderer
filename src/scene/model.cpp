#include <scene/model.h>

// std
#include <cassert>
#include <cstring>

namespace mvr
{

Model::Model(VulkanDevice &device, const std::vector<Vertex> &vertices) : device{device}
{
	createVertexBuffers(vertices);
}

void Model::bind(vk::CommandBuffer commandBuffer)
{
	commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
}

void Model::draw(vk::CommandBuffer commandBuffer)
{
	commandBuffer.draw(vertexCount, 1, 0, 0);
}

void Model::createVertexBuffers(const std::vector<Vertex> &vertices)
{
	vertexCount = static_cast<uint32_t>(vertices.size());

	vk::BufferCreateInfo bufferInfo = {
	    .size        = sizeof(vertices[0]) * vertices.size(),
	    .usage       = vk::BufferUsageFlagBits::eVertexBuffer,
	    .sharingMode = vk::SharingMode::eExclusive,
	};

	vertexBuffer = vk::raii::Buffer(device.device(), bufferInfo);

	vk::MemoryRequirements memRequirements = vertexBuffer.getMemoryRequirements();
	vk::MemoryAllocateInfo memAllocateInfo = {
	    .allocationSize  = memRequirements.size,
	    .memoryTypeIndex = device.findMemoryType(
	        memRequirements.memoryTypeBits,
	        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
	};
	vertexBufferMemory = vk::raii::DeviceMemory(device.device(), memAllocateInfo);

	vertexBuffer.bindMemory(*vertexBufferMemory, 0);

	void *data = vertexBufferMemory.mapMemory(0, bufferInfo.size);
	memcpy(data, vertices.data(), bufferInfo.size);
	vertexBufferMemory.unmapMemory();
}

}        // namespace mvr