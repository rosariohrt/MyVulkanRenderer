#include "ui/imgui_renderer.h"

// std
#include <tuple>

namespace mvr
{
ImGuiRenderer::ImGuiRenderer(VulkanDevice &device) : device{device}
{
	std::tie(vertexBuffer, vertexBufferMemory) =
	    device.createBuffer(1,
	                        vk::BufferUsageFlagBits::eVertexBuffer,
	                        vk::MemoryPropertyFlagBits::eHostVisible |
	                            vk::MemoryPropertyFlagBits::eHostCoherent);
	std::tie(indexBuffer, indexBufferMemory) =
	    device.createBuffer(1,
	                        vk::BufferUsageFlagBits::eIndexBuffer,
	                        vk::MemoryPropertyFlagBits::eHostVisible |
	                            vk::MemoryPropertyFlagBits::eHostCoherent);

	renderingInfo.colorAttachmentCount    = 1;
	renderingInfo.pColorAttachmentFormats = &colorFormat;
}

}        // namespace mvr
