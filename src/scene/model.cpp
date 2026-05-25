#include <scene/model.h>

// std
#include <cassert>
#include <cstring>

namespace mvr
{

Model::Model(VulkanDevice                &device,
             const std::vector<Vertex>   &vertices,
             const std::vector<uint16_t> &indices) :
    device{device}
{
	createVertexBuffer(vertices);
	createIndexBuffer(indices);
	createUniformBuffers();
}

void Model::bind(vk::CommandBuffer commandBuffer)
{
	commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
	commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
}

void Model::draw(vk::CommandBuffer commandBuffer)
{
	commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void Model::createVertexBuffer(const std::vector<Vertex> &vertices)
{
	vertexCount               = static_cast<uint32_t>(vertices.size());
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

void Model::createIndexBuffer(const std::vector<uint16_t> &indices)
{
	indexCount                = static_cast<uint32_t>(indices.size());
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	auto [stagingBuffer, stagingBufferMemory] = device.createBuffer(
	    bufferSize,
	    vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(dataStaging, indices.data(), bufferSize);
	stagingBufferMemory.unmapMemory();

	std::tie(indexBuffer, indexBufferMemory) = device.createBuffer(
	    bufferSize,
	    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	    vk::MemoryPropertyFlagBits::eDeviceLocal);
	device.copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Model::createUniformBuffers()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

		auto [uniformBuffer, uniformBufferMemory] = device.createBuffer(
		    bufferSize,
		    vk::BufferUsageFlagBits::eUniformBuffer,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		uniformBuffers.push_back(std::move(uniformBuffer));
		uniformBuffersMemory.push_back(std::move(uniformBufferMemory));
		uniformBuffersMapped.push_back(uniformBuffersMemory[i].mapMemory(0, bufferSize));        // persistent mapping
	}
}

}        // namespace mvr