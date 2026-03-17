#include "first_app.h"

// std
#include <array>
#include <cstdint>
#include <stdexcept>

namespace mvr
{

FirstApp::FirstApp()
{
	loadModel();
	createPipelineLayout();
	createPipeline();
	createCommandBuffers();
}

FirstApp::~FirstApp()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void FirstApp::run()
{
	while (!window.shouldClose()) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(device.device());
}

void FirstApp::loadModel()
{
	std::vector<Model::Vertex> vertices{
	    {{0.0f, -0.5f}},
	    {{0.5f, 0.5f}},
	    {{-0.5f, 0.5f}},
	};

	model = std::make_unique<Model>(device, vertices);
}

void FirstApp::createPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = 0;
	pipelineLayoutInfo.pSetLayouts            = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges    = nullptr;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void FirstApp::createPipeline()
{
	auto pipelineConfig           = Pipeline::defaultPipelineConfigInfo(swapChain.width(), swapChain.height());
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipelineConfig.renderPass     = swapChain.getRenderPass();
	pipeline                      = std::make_unique<Pipeline>(device,
	                                                           "../shaders/simple_shader.vert.spv",
	                                                           "../shaders/simple_shader.frag.spv",
	                                                           pipelineConfig);
}

void FirstApp::createCommandBuffers()
{
	commandBuffers.resize(swapChain.imageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (int i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo remderPassInfo{};
		remderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		remderPassInfo.renderPass        = swapChain.getRenderPass();
		remderPassInfo.framebuffer       = swapChain.getFrameBuffer(i);
		remderPassInfo.renderArea.offset = {0, 0};
		remderPassInfo.renderArea.extent = swapChain.getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color           = {0.1f, 0.1f, 0.1f, 1.0f};
		clearValues[1].depthStencil    = {1.0f, 0};
		remderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		remderPassInfo.pClearValues    = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &remderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		pipeline->bind(commandBuffers[i]);
		model->bind(commandBuffers[i]);
		model->draw(commandBuffers[i]);

		vkCmdEndRenderPass(commandBuffers[i]);
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to end recording command buffer!");
		}
	}
}

void FirstApp::drawFrame()
{
	uint32_t imageIndex;
	auto     result = swapChain.acquireNextImage(&imageIndex);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	result = swapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to submit command buffer!");
	}
}

}        // namespace mvr