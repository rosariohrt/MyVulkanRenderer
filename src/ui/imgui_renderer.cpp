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

void ImGuiRenderer::init(vk::Extent2D extent)
{
	// Initialize ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Configure ImGui
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Inform ImGui that we support the new texture update protocol (v1.92+)
	// This enables support for dynamic font textures and multiple texture
	// atlases
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	io.DisplaySize             = ImVec2(static_cast<float>(extent.width),
                            static_cast<float>(extent.height));
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	vulkanStyle                                = ImGui::GetStyle();
	vulkanStyle.Colors[ImGuiCol_TitleBg]       = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	vulkanStyle.Colors[ImGuiCol_MenuBarBg]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_Header]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_CheckMark]     = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	// Apply default style
	setStyle(0);
}

void ImGuiRenderer::setStyle(uint32_t index)
{
	ImGuiStyle &style = ImGui::GetStyle();

	switch (index) {
		case 0:
			// Custom Vulkan style
			style = vulkanStyle;
			break;
		case 1:
			// Classic Style
			ImGui::StyleColorsClassic();
			break;
		case 2:
			// Dark style
			ImGui::StyleColorsDark();
			break;
		case 3:
			// Light style
			ImGui::StyleColorsLight();
			break;
	}
}

}        // namespace mvr
