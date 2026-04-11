#include "swap_chain.h"

// std
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace mvr
{

// Constructor & Destructor

SwapChain::SwapChain(VulkanDevice &device, VkExtent2D windowExtent) : device{device}, windowExtent{windowExtent}
{
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDepthResources();
	createFramebuffers();
	createSyncObjects();
}

SwapChain::~SwapChain()
{
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(*device.device(), imageView, nullptr);
	}
	swapChainImageViews.clear();

	for (int i = 0; i < depthImages.size(); i++) {
		vkDestroyImageView(*device.device(), depthImageViews[i], nullptr);
		vkDestroyImage(*device.device(), depthImages[i], nullptr);
		vkFreeMemory(*device.device(), depthImageMemorys[i], nullptr);
	}

	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(*device.device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(*device.device(), renderPass, nullptr);

	// cleanup synchronization objects
	for (size_t i = 0; i < renderFinishedSemaphores.size(); i++) {
		vkDestroySemaphore(*device.device(), renderFinishedSemaphores[i], nullptr);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(*device.device(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(*device.device(), inFlightFences[i], nullptr);
	}
}

// Public Methods

VkResult SwapChain::acquireNextImage(uint32_t *imageIndex)
{
	vkWaitForFences(
	    *device.device(),
	    1,
	    &inFlightFences[currentFrame],
	    VK_TRUE,
	    std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(
	    *device.device(),
	    *swapChain,
	    std::numeric_limits<uint64_t>::max(),
	    imageAvailableSemaphores[currentFrame],        // must be a not signaled semaphore
	    VK_NULL_HANDLE,
	    imageIndex);

	return result;
}

VkResult SwapChain::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex)
{
	if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(*device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore          waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount         = 1;
	submitInfo.pWaitSemaphores            = waitSemaphores;
	submitInfo.pWaitDstStageMask          = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = buffers;

	VkSemaphore signalSemaphores[]  = {renderFinishedSemaphores[*imageIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = signalSemaphores;

	vkResetFences(*device.device(), 1, &inFlightFences[currentFrame]);
	if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) !=
	    VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = signalSemaphores;

	VkSwapchainKHR swapChains[] = {*swapChain};
	presentInfo.swapchainCount  = 1;
	presentInfo.pSwapchains     = swapChains;

	presentInfo.pImageIndices = imageIndex;

	auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}

VkFormat SwapChain::findDepthFormat()
{
	return device.findSupportedFormat(
	    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

// Private Init Methods

void SwapChain::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

	swapChainSurfaceFormat         = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	swapChainExtent                = chooseSwapExtent(swapChainSupport.capabilities);
	uint32_t imageCount            = chooseSwapMinImageCount(swapChainSupport.capabilities);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo = {
	    .surface          = device.surface(),
	    .minImageCount    = imageCount,
	    .imageFormat      = swapChainSurfaceFormat.format,
	    .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
	    .imageExtent      = swapChainExtent,
	    .imageArrayLayers = 1,
	    .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
	    .imageSharingMode = vk::SharingMode::eExclusive,
	    .preTransform     = swapChainSupport.capabilities.currentTransform,
	    .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
	    .presentMode      = presentMode,
	    .clipped          = true};

	QueueFamilyIndices indices              = device.findPhysicalQueueFamilies();
	uint32_t           queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily) {
		swapChainCreateInfo.imageSharingMode    = vk::SharingMode::eConcurrent;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	swapChain       = vk::raii::SwapchainKHR(device.device(), swapChainCreateInfo);
	swapChainImages = swapChain.getImages();
}

void SwapChain::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                           = swapChainImages[i];
		viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                          = static_cast<VkFormat>(swapChainSurfaceFormat.format);
		viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel   = 0;
		viewInfo.subresourceRange.levelCount     = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(*device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) !=
		    VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::createRenderPass()
{
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format         = findDepthFormat();
	depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format                  = getSwapChainSurfaceFormat();
	colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment            = 0;
	colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass    = {};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask       = 0;
	dependency.srcStageMask =
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstSubpass = 0;
	dependency.dstStageMask =
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask =
	    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments    = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo                 renderPassInfo = {};
	renderPassInfo.sType                                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount                        = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments                           = attachments.data();
	renderPassInfo.subpassCount                           = 1;
	renderPassInfo.pSubpasses                             = &subpass;
	renderPassInfo.dependencyCount                        = 1;
	renderPassInfo.pDependencies                          = &dependency;

	if (vkCreateRenderPass(*device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void SwapChain::createDepthResources()
{
	VkFormat   depthFormat     = findDepthFormat();
	VkExtent2D swapChainExtent = getSwapChainExtent();

	depthImages.resize(imageCount());
	depthImageMemorys.resize(imageCount());
	depthImageViews.resize(imageCount());

	for (int i = 0; i < depthImages.size(); i++) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType     = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width  = swapChainExtent.width;
		imageInfo.extent.height = swapChainExtent.height;
		imageInfo.extent.depth  = 1;
		imageInfo.mipLevels     = 1;
		imageInfo.arrayLayers   = 1;
		imageInfo.format        = depthFormat;
		imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags         = 0;

		device.createImageWithInfo(
		    imageInfo,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    depthImages[i],
		    depthImageMemorys[i]);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                           = depthImages[i];
		viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                          = depthFormat;
		viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel   = 0;
		viewInfo.subresourceRange.levelCount     = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(*device.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::createFramebuffers()
{
	swapChainFramebuffers.resize(imageCount());
	for (size_t i = 0; i < imageCount(); i++) {
		std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};

		VkExtent2D              swapChainExtent = getSwapChainExtent();
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass              = renderPass;
		framebufferInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments            = attachments.data();
		framebufferInfo.width                   = swapChainExtent.width;
		framebufferInfo.height                  = swapChainExtent.height;
		framebufferInfo.layers                  = 1;

		if (vkCreateFramebuffer(
		        *device.device(),
		        &framebufferInfo,
		        nullptr,
		        &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void SwapChain::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(imageCount());
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < imageCount(); i++) {
		if (vkCreateSemaphore(*device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
		    VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(*device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
		        VK_SUCCESS ||
		    vkCreateFence(*device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

// Helper Methods

vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(
    std::vector<vk::SurfaceFormatKHR> const &availableFormats)
{
	assert(!availableFormats.empty());
	const auto formatIt = std::ranges::find_if(availableFormats, [](const auto &format) {
		return format.format == vk::Format::eB8G8R8A8Srgb &&
		       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
	});
	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR SwapChain::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
	assert(std::ranges::any_of(availablePresentModes, [](const auto &presentMode) {
		return presentMode == vk::PresentModeKHR::eFifo;
	}));

	return std::ranges::any_of(availablePresentModes, [](const auto &presentMode) {
		return presentMode == vk::PresentModeKHR::eMailbox;
	}) ?
	           vk::PresentModeKHR::eMailbox :
	           vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	int width, height;
	device.getWindow().getFrameBufferSize(&width, &height);
	// TODO: This implementation violates the Law of Demeter.

	return {
	    std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
	    std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
	};
}

uint32_t SwapChain::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities)
{
	auto minImageCount = std::max(3u, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
		return capabilities.maxImageCount;
	}

	return minImageCount;
}

}        // namespace mvr
