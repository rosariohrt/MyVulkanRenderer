#include "first_app.h"

// std
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
}

void FirstApp::run()
{
	while (!window.shouldClose()) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(*device.device());
}

void FirstApp::loadModel()
{
	std::vector<Model::Vertex> vertices{
	    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	};

	model = std::make_unique<Model>(device, vertices);
}

void FirstApp::createPipelineLayout()
{
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
	    .setLayoutCount         = 0,
	    .pushConstantRangeCount = 0,
	};

	pipelineLayout = vk::raii::PipelineLayout(device.device(), pipelineLayoutInfo);
}

void FirstApp::createPipeline()
{
	auto pipelineConfig           = Pipeline::defaultPipelineConfigInfo(swapChain.getSwapChainSurfaceFormat());
	pipelineConfig.pipelineLayout = *pipelineLayout;
	pipeline                      = std::make_unique<Pipeline>(device,
	                                                           "../shaders/simple_shader.vert.spv",
	                                                           "../shaders/simple_shader.frag.spv",
	                                                           pipelineConfig);
}

void FirstApp::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo allocInfo = {
	    .commandPool        = device.getCommandPool(),
	    .level              = vk::CommandBufferLevel::ePrimary,
	    .commandBufferCount = SwapChain::MAX_FRAMES_IN_FLIGHT,
	};

	commandBuffers = vk::raii::CommandBuffers(device.device(), allocInfo);
}

void FirstApp::recordCommandBuffer(uint32_t imageIndex)
{
	auto &commandBuffer = commandBuffers[frameIndex];
	commandBuffer.begin({});

	transitionImageLayout(
	    imageIndex,
	    vk::ImageLayout::eUndefined,
	    vk::ImageLayout::eColorAttachmentOptimal,
	    vk::AccessFlagBits2{},                                     // srcAccessMask
	    vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
	    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
	    vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
	);

	vk::ClearValue              clearColor     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	vk::RenderingAttachmentInfo attachmentInfo = {
	    .imageView   = swapChain.getImageView(imageIndex),
	    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
	    .loadOp      = vk::AttachmentLoadOp::eClear,
	    .storeOp     = vk::AttachmentStoreOp::eStore,
	    .clearValue  = clearColor,
	};
	vk::RenderingInfo renderingInfo = {
	    .renderArea           = {.offset = {0, 0}, .extent = swapChain.getSwapChainExtent()},
	    .layerCount           = 1,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &attachmentInfo,
	    .pDepthAttachment     = nullptr,
	    .pStencilAttachment   = nullptr,
	};

	commandBuffer.beginRendering(renderingInfo);
	pipeline->bind(commandBuffer);
	commandBuffer.setViewport(0, swapChain.getViewport());
	commandBuffer.setScissor(0, swapChain.getScissor());
	model->bind(commandBuffer);
	model->draw(commandBuffer);
	commandBuffer.endRendering();

	transitionImageLayout(
	    imageIndex,
	    vk::ImageLayout::eColorAttachmentOptimal,
	    vk::ImageLayout::ePresentSrcKHR,
	    vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
	    vk::AccessFlagBits2{},                                     // dstAccessMask
	    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
	    vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
	);

	commandBuffer.end();
}

void FirstApp::transitionImageLayout(
    uint32_t                imageIndex,
    vk::ImageLayout         oldLayout,
    vk::ImageLayout         newLayout,
    vk::AccessFlags2        srcAccessMask,
    vk::AccessFlags2        dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask)
{
	vk::ImageMemoryBarrier2 barrier = {
	    .srcStageMask        = srcStageMask,
	    .srcAccessMask       = srcAccessMask,
	    .dstStageMask        = dstStageMask,
	    .dstAccessMask       = dstAccessMask,
	    .oldLayout           = oldLayout,
	    .newLayout           = newLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image               = swapChain.getImage(imageIndex),
	    .subresourceRange    = {
	           .aspectMask     = vk::ImageAspectFlagBits::eColor,
	           .baseMipLevel   = 0,
	           .levelCount     = 1,
	           .baseArrayLayer = 0,
	           .layerCount     = 1,
        },
	};

	vk::DependencyInfo dependencyInfo = {
	    .dependencyFlags         = {},
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers    = &barrier,
	};

	commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

void FirstApp::drawFrame()
{
	auto [result, imageIndex] = swapChain.acquireNextImage(frameIndex);

	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	commandBuffers[frameIndex].reset();
	recordCommandBuffer(imageIndex);

	result = swapChain.submitCommandBuffers(commandBuffers[frameIndex], imageIndex, frameIndex);
	switch (result) {
		case vk::Result::eSuccess:
			break;
		case vk::Result::eSuboptimalKHR:
			// Optionally handle suboptimal swap chain
			break;
		default:
			throw std::runtime_error("failed to present swap chain image!");
	}

	frameIndex = (frameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}

}        // namespace mvr