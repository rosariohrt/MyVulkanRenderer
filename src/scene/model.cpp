#include "scene/model.h"
#include "constants.h"
#include "ubo.h"

// std
#include <cassert>
#include <cstring>
#include <iostream>

// libs
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace mvr
{

Model::Model(VulkanDevice &device, const std::string &path) : device{device}
{
	std::vector<Model::Vertex> vertices;
	std::vector<uint32_t>      indices;
	loadGltfModel(vertices, indices, path);

	createVertexBuffer(vertices);
	createIndexBuffer(indices);
	createUniformBuffers();
}

void Model::loadGltfModel(std::vector<Vertex>   &vertices,
                          std::vector<uint32_t> &indices,
                          const std::string     &path)
{
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err, warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

	if (!warn.empty()) {
		std::cout << "glTF warning: " << warn << std::endl;
	}
	if (!err.empty()) {
		std::cout << "glTF error: " << err << std::endl;
	}
	if (!ret) {
		throw std::runtime_error("Failed to load glTF model: " + path);
	}

	// Process all meshes in the model
	for (const auto &mesh : model.meshes) {
		for (const auto &primitive : mesh.primitives) {
			// Get indices
			const tinygltf::Accessor &indexAccessor =
			    model.accessors[primitive.indices];
			const tinygltf::BufferView &indexBufferView =
			    model.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer &indexBuffer =
			    model.buffers[indexBufferView.buffer];

			// Get vertex positions
			const tinygltf::Accessor &posAccessor =
			    model.accessors[primitive.attributes.at("POSITION")];
			const tinygltf::BufferView &posBufferView =
			    model.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer &posBuffer =
			    model.buffers[posBufferView.buffer];

			// Get normals
			bool hasNormals = primitive.attributes.find("NORMAL") !=
			                  primitive.attributes.end();
			const tinygltf::Accessor   *normAccessor   = nullptr;
			const tinygltf::BufferView *normBufferView = nullptr;
			const tinygltf::Buffer     *normBuffer     = nullptr;

			if (hasNormals) {
				normAccessor =
				    &model.accessors[primitive.attributes.at("NORMAL")];
				normBufferView = &model.bufferViews[normAccessor->bufferView];
				normBuffer     = &model.buffers[normBufferView->buffer];
			}

			// Get texture coordinates
			bool hasTexCoords = primitive.attributes.find("TEXCOORD_0") !=
			                    primitive.attributes.end();
			const tinygltf::Accessor   *texCoordAccessor   = nullptr;
			const tinygltf::BufferView *texCoordBufferView = nullptr;
			const tinygltf::Buffer     *texCoordBuffer     = nullptr;

			if (hasTexCoords) {
				texCoordAccessor =
				    &model.accessors[primitive.attributes.at("TEXCOORD_0")];
				texCoordBufferView =
				    &model.bufferViews[texCoordAccessor->bufferView];
				texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
			}

			// Track vertex offset for index remapping
			uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

			// process all vertices
			for (size_t i = 0; i < posAccessor.count; ++i) {
				Vertex vertex{};

				const float *pos = reinterpret_cast<const float *>(
				    &posBuffer
				         .data[posBufferView.byteOffset +
				               posAccessor.byteOffset + i * 3 * sizeof(float)]);
				vertex.pos = glm::vec3{pos[0], pos[1], pos[2]};

				if (hasNormals) {
					const float *normal = reinterpret_cast<const float *>(
					    &normBuffer->data[normBufferView->byteOffset +
					                      normAccessor->byteOffset +
					                      i * 3 * sizeof(float)]);
					vertex.normal = glm::vec3{normal[0], normal[1], normal[2]};
				}

				if (hasTexCoords) {
					const float *texCoord = reinterpret_cast<const float *>(
					    &texCoordBuffer->data[texCoordBufferView->byteOffset +
					                          texCoordAccessor->byteOffset +
					                          i * 2 * sizeof(float)]);
					vertex.texCoord = glm::vec2{texCoord[0], texCoord[1]};
				}

				vertices.push_back(vertex);
			}

			const unsigned char *indexData =
			    &indexBuffer.data[indexBufferView.byteOffset +
			                      indexAccessor.byteOffset];
			size_t indexCount = indexAccessor.count;

			indices.reserve(indices.size() + indexCount);

			// process all indices
			for (size_t i = 0; i < indexCount; ++i) {
				uint32_t index = 0;

				switch (indexAccessor.componentType) {
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						index = static_cast<uint32_t>(
						    *reinterpret_cast<const uint8_t *>(
						        indexData + i * sizeof(uint8_t)));
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						index = static_cast<uint32_t>(
						    *reinterpret_cast<const uint16_t *>(
						        indexData + i * sizeof(uint16_t)));
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						index = *reinterpret_cast<const uint32_t *>(
						    indexData + i * sizeof(uint32_t));
						break;
					default:
						throw std::runtime_error(
						    "Unsupported index component type in glTF model");
				}

				indices.push_back(baseVertex + index);
			}
		}
	}
}

void Model::bind(vk::CommandBuffer commandBuffer)
{
	commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
	commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint32);
}

void Model::draw(vk::CommandBuffer commandBuffer)
{
	commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

vk::DescriptorBufferInfo Model::getDescriptorBufferInfo(size_t index)
{
	vk::DescriptorBufferInfo bufferInfo = {
	    .buffer = *uniformBuffers[index],
	    .offset = 0,
	    .range  = sizeof(UniformBufferObject),
	};

	return bufferInfo;
}

void Model::createVertexBuffer(const std::vector<Vertex> &vertices)
{
	vertexCount               = static_cast<uint32_t>(vertices.size());
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	auto [stagingBuffer, stagingBufferMemory] =
	    device.createBuffer(bufferSize,
	                        vk::BufferUsageFlagBits::eTransferSrc,
	                        vk::MemoryPropertyFlagBits::eHostVisible |
	                            vk::MemoryPropertyFlagBits::eHostCoherent);

	void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(dataStaging, vertices.data(), bufferSize);
	stagingBufferMemory.unmapMemory();

	std::tie(vertexBuffer, vertexBufferMemory) =
	    device.createBuffer(bufferSize,
	                        vk::BufferUsageFlagBits::eTransferDst |
	                            vk::BufferUsageFlagBits::eVertexBuffer,
	                        vk::MemoryPropertyFlagBits::eDeviceLocal);
	device.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void Model::createIndexBuffer(const std::vector<uint32_t> &indices)
{
	indexCount                = static_cast<uint32_t>(indices.size());
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	auto [stagingBuffer, stagingBufferMemory] =
	    device.createBuffer(bufferSize,
	                        vk::BufferUsageFlagBits::eTransferSrc,
	                        vk::MemoryPropertyFlagBits::eHostVisible |
	                            vk::MemoryPropertyFlagBits::eHostCoherent);

	void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(dataStaging, indices.data(), bufferSize);
	stagingBufferMemory.unmapMemory();

	std::tie(indexBuffer, indexBufferMemory) =
	    device.createBuffer(bufferSize,
	                        vk::BufferUsageFlagBits::eTransferDst |
	                            vk::BufferUsageFlagBits::eIndexBuffer,
	                        vk::MemoryPropertyFlagBits::eDeviceLocal);
	device.copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Model::createUniformBuffers()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

		auto [uniformBuffer, uniformBufferMemory] =
		    device.createBuffer(bufferSize,
		                        vk::BufferUsageFlagBits::eUniformBuffer,
		                        vk::MemoryPropertyFlagBits::eHostVisible |
		                            vk::MemoryPropertyFlagBits::eHostCoherent);

		uniformBuffers.push_back(std::move(uniformBuffer));
		uniformBuffersMemory.push_back(std::move(uniformBufferMemory));
		uniformBuffersMapped.push_back(uniformBuffersMemory[i].mapMemory(
		    0, bufferSize));        // persistent mapping
	}
}

}        // namespace mvr
