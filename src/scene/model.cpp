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
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	auto [stagingBuffer, stagingBufferMemory] = device.createBuffer(
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(dataStaging, vertices.data(), bufferSize);
	stagingBufferMemory.unmapMemory();

	std::tie(vertexBuffer, vertexBufferMemory) = device.createBuffer(
		bufferSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
	    vk::MemoryPropertyFlagBits::eDeviceLocal);
	device.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

}        // namespace mvr