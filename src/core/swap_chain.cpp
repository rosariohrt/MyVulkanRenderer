#include "swap_chain.h"

// std
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace mvr
{

// Constructor & Destructor

SwapChain::SwapChain(VulkanDevice &device, VkExtent2D windowExtent) : device{device}, windowExtent{windowExtent}
{
	createSwapChain();
	createImageViews();
	createDepthResources();
	createSyncObjects();
}

SwapChain::~SwapChain()
{
	for (int i = 0; i < depthImages.size(); i++) {
		vkDestroyImageView(*device.device(), depthImageViews[i], nullptr);
		vkDestroyImage(*device.device(), depthImages[i], nullptr);
		vkFreeMemory(*device.device(), depthImageMemorys[i], nullptr);
	}
}

// Public Methods

std::pair<vk::Result, uint32_t> SwapChain::acquireNextImage(uint32_t frameIndex)
{
	auto fenceResult = device.device().waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
	if (fenceResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to wait for fence!");
	}
	device.device().resetFences(*inFlightFences[frameIndex]);

	auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[frameIndex], nullptr);

	return {result, imageIndex};
}

vk::Result SwapChain::submitCommandBuffers(const vk::raii::CommandBuffer &commandBuffer, uint32_t imageIndex, uint32_t frameIndex)
{
	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	const vk::SubmitInfo   submitInfo{
	    .waitSemaphoreCount   = 1,
	    .pWaitSemaphores      = &*imageAvailableSemaphores[frameIndex],
	    .pWaitDstStageMask    = &waitDestinationStageMask,
	    .commandBufferCount   = 1,
	    .pCommandBuffers      = &*commandBuffer,
	    .signalSemaphoreCount = 1,
	    .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
	};

	device.graphicsQueue().submit(submitInfo, *inFlightFences[frameIndex]);

	const vk::PresentInfoKHR presentInfoKHR{
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
	    .swapchainCount     = 1,
	    .pSwapchains        = &*swapChain,
	    .pImageIndices      = &imageIndex
	};

	return device.presentQueue().presentKHR(presentInfoKHR);
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
	assert(swapChainImageViews.empty());

	vk::ImageViewCreateInfo imageviewCreateInfo = {
	    .viewType         = vk::ImageViewType::e2D,
	    .format           = swapChainSurfaceFormat.format,
	    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

	for (auto &image : swapChainImages) {
		imageviewCreateInfo.image = image;
		swapChainImageViews.emplace_back(device.device(), imageviewCreateInfo);
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
		    vk::MemoryPropertyFlagBits::eDeviceLocal,
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

void SwapChain::createSyncObjects()
{
	assert(renderFinishedSemaphores.empty() && imageAvailableSemaphores.empty() && inFlightFences.empty());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		renderFinishedSemaphores.emplace_back(device.device(), vk::SemaphoreCreateInfo{});
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		imageAvailableSemaphores.emplace_back(device.device(), vk::SemaphoreCreateInfo{});
		inFlightFences.emplace_back(device.device(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
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
