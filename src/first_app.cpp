#include "first_app.h"

// std
#include <stdexcept>

namespace mvr
{

FirstApp::FirstApp()
{
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
	}
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
}

void FirstApp::drawFrame()
{
}

}        // namespace mvr