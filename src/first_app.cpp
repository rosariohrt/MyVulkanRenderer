#include "first_app.h"
#include "constants.h"

// std
#include <array>
#include <cstdint>
#include <stdexcept>

namespace mvr
{

FirstApp::FirstApp()
{
	loadModel();
	loadTexture();
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSets();
	createPipelineLayout();
	recreateSwapChain();
	createCommandBuffers();
}

FirstApp::~FirstApp()
{}

void FirstApp::run()
{
	while (!window.shouldClose()) {
		glfwPollEvents();
		drawFrame();
	}

	device.device().waitIdle();
}

void FirstApp::loadModel()
{
	const std::vector<Model::Vertex> vertices{
	    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
	};

	const std::vector<uint16_t> indices = {
	    0,
	    1,
	    2,
	    2,
	    3,
	    0,
	};

	model = std::make_unique<Model>(device, vertices, indices);
}

void FirstApp::loadTexture()
{
	texture = std::make_unique<Texture>(device, "textures/texture.jpg");
}

void FirstApp::createDescriptorSetLayout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings   = {{
	    {
	        .binding         = 0,
	        .descriptorType  = vk::DescriptorType::eUniformBuffer,
	        .descriptorCount = 1,
	        .stageFlags      = vk::ShaderStageFlagBits::eVertex,
	    },
	    {
	        .binding         = 1,
	        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
	        .descriptorCount = 1,
	        .stageFlags      = vk::ShaderStageFlagBits::eFragment,
	    },
	}};
	vk::DescriptorSetLayoutCreateInfo             layoutInfo = {
	    .bindingCount = static_cast<uint32_t>(bindings.size()),
	    .pBindings    = bindings.data(),
	};

	descriptorSetLayout =
	    vk::raii::DescriptorSetLayout(device.device(), layoutInfo);
}

void FirstApp::createDescriptorPool()
{
	std::array<vk::DescriptorPoolSize, 2> poolSize = {{
	    {
	        .type            = vk::DescriptorType::eUniformBuffer,
	        .descriptorCount = MAX_FRAMES_IN_FLIGHT,
	    },
	    {
	        .type            = vk::DescriptorType::eCombinedImageSampler,
	        .descriptorCount = MAX_FRAMES_IN_FLIGHT,
	    },
	}};

	vk::DescriptorPoolCreateInfo poolInfo = {
	    .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	    .maxSets       = MAX_FRAMES_IN_FLIGHT,
	    .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
	    .pPoolSizes    = poolSize.data(),
	};
	descriptorPool = vk::raii::DescriptorPool(device.device(), poolInfo);
}

void FirstApp::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
	                                             *descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo = {
	    .descriptorPool     = *descriptorPool,
	    .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
	    .pSetLayouts        = layouts.data(),
	};
	descriptorSets = device.device().allocateDescriptorSets(allocInfo);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo bufferInfo = model->getDescriptorBufferInfo(i);
		vk::DescriptorImageInfo  imageInfo  = texture->getDescriptorImageInfo();

		std::array<vk::WriteDescriptorSet, 2> descriptorWrite = {{
		    {
		        .dstSet          = *descriptorSets[i],
		        .dstBinding      = 0,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType  = vk::DescriptorType::eUniformBuffer,
		        .pBufferInfo     = &bufferInfo,
		    },
		    {
		        .dstSet          = *descriptorSets[i],
		        .dstBinding      = 1,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
		        .pImageInfo      = &imageInfo,
		    },
		}};
		device.device().updateDescriptorSets(descriptorWrite, {});
	}
}

void FirstApp::createPipelineLayout()
{
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
	    .setLayoutCount         = 1,
	    .pSetLayouts            = &*descriptorSetLayout,
	    .pushConstantRangeCount = 0,
	};

	pipelineLayout =
	    vk::raii::PipelineLayout(device.device(), pipelineLayoutInfo);
}

void FirstApp::createPipeline()
{
	auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(
	    swapChain->getSwapChainSurfaceFormat());
	pipelineConfig.pipelineLayout = *pipelineLayout;
	pipeline = std::make_unique<Pipeline>(device,
	                                      "shaders/simple_shader.vert.spv",
	                                      "shaders/simple_shader.frag.spv",
	                                      pipelineConfig);
}

void FirstApp::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo allocInfo = {
	    .commandPool        = device.getCommandPool(),
	    .level              = vk::CommandBufferLevel::ePrimary,
	    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
	};

	commandBuffers = vk::raii::CommandBuffers(device.device(), allocInfo);
}

void FirstApp::recreateSwapChain()
{
	auto extent = window.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = window.getExtent();
		glfwWaitEvents();
	}

	device.device().waitIdle();
	swapChain.reset();
	swapChain = std::make_unique<SwapChain>(device, extent);
	createPipeline();
}

void FirstApp::updateUniformBuffer(uint32_t frameIndex)
{
	static auto startTime   = std::chrono::high_resolution_clock::now();
	auto        currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(
	                 currentTime - startTime)
	                 .count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f),
	                        time * glm::radians(90.0f),
	                        glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
	                        glm::vec3(0.0f, 0.0f, 0.0f),
	                        glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj  = glm::perspective(
	    glm::radians(45.0f), swapChain->extentAspectRatio(), 0.1f, 10.0f);
	ubo.proj[1][1] *=
	    -1;        // Invert Y coordinate for Vulkan's coordinate system

	memcpy(model->getUniformBuffersMapped(frameIndex), &ubo, sizeof(ubo));
}

void FirstApp::recordCommandBuffer(uint32_t imageIndex)
{
	auto &commandBuffer = commandBuffers[frameIndex];
	commandBuffer.begin({});

	transitionImageLayout(
	    imageIndex,
	    vk::ImageLayout::eUndefined,
	    vk::ImageLayout::eColorAttachmentOptimal,
	    vk::AccessFlagBits2::eNone,                        // srcAccessMask
	    vk::AccessFlagBits2::eColorAttachmentWrite,        // dstAccessMask
	    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
	    vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
	);

	vk::ClearValue clearColor = vk::ClearColorValue(0.1f, 0.1f, 0.1f, 1.0f);
	vk::RenderingAttachmentInfo attachmentInfo = {
	    .imageView   = swapChain->getImageView(imageIndex),
	    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
	    .loadOp      = vk::AttachmentLoadOp::eClear,
	    .storeOp     = vk::AttachmentStoreOp::eStore,
	    .clearValue  = clearColor,
	};
	vk::RenderingInfo renderingInfo = {
	    .renderArea           = {.offset = {0, 0},
	                             .extent = swapChain->getSwapChainExtent()},
	    .layerCount           = 1,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &attachmentInfo,
	    .pDepthAttachment     = nullptr,
	    .pStencilAttachment   = nullptr,
	};

	commandBuffer.beginRendering(renderingInfo);
	pipeline->bind(commandBuffer);
	commandBuffer.setViewport(0, swapChain->getViewport());
	commandBuffer.setScissor(0, swapChain->getScissor());
	model->bind(commandBuffer);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
	                                 *pipelineLayout,
	                                 0,
	                                 *descriptorSets[frameIndex],
	                                 nullptr);
	model->draw(commandBuffer);
	commandBuffer.endRendering();

	transitionImageLayout(
	    imageIndex,
	    vk::ImageLayout::eColorAttachmentOptimal,
	    vk::ImageLayout::ePresentSrcKHR,
	    vk::AccessFlagBits2::eColorAttachmentWrite,        // srcAccessMask
	    vk::AccessFlagBits2::eNone,                        // dstAccessMask
	    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
	    vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
	);

	commandBuffer.end();
}

void FirstApp::transitionImageLayout(uint32_t                imageIndex,
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
	    .image               = swapChain->getImage(imageIndex),
	    .subresourceRange =
	        {
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
	auto [result, imageIndex] = swapChain->acquireNextImage(frameIndex);

	if (result == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapChain();
		return;
	}

	if (result != vk::Result::eSuccess &&
	    result != vk::Result::eSuboptimalKHR) {
		assert(result == vk::Result::eSuccess ||
		       result == vk::Result::eNotReady);
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	updateUniformBuffer(frameIndex);

	// Only reset the fence if we are submitting work
	swapChain->resetFences(frameIndex);

	commandBuffers[frameIndex].reset();
	recordCommandBuffer(imageIndex);

	result = swapChain->submitCommandBuffers(
	    commandBuffers[frameIndex], imageIndex, frameIndex);
	if (result == vk::Result::eErrorOutOfDateKHR ||
	    result == vk::Result::eSuboptimalKHR || window.wasWindowResized()) {
		window.resetWindowResizedFlag();
		recreateSwapChain();
	} else if (result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

}        // namespace mvr