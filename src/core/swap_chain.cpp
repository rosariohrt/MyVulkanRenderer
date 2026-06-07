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

namespace
{
vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
	assert(!availableFormats.empty());
	const auto formatIt =
	    std::ranges::find_if(availableFormats, [](const auto &format) {
		    return format.format == vk::Format::eB8G8R8A8Srgb &&
		           format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
	    });
	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
	assert(
	    std::ranges::any_of(availablePresentModes, [](const auto &presentMode) {
		    return presentMode == vk::PresentModeKHR::eFifo;
	    }));

	return std::ranges::any_of(availablePresentModes,
	                           [](const auto &presentMode) {
		                           return presentMode ==
		                                  vk::PresentModeKHR::eMailbox;
	                           }) ?
	           vk::PresentModeKHR::eMailbox :
	           vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities,
                              vk::Extent2D                      windowExtent)
{
	if (capabilities.currentExtent.width !=
	    std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	int width  = windowExtent.width;
	int height = windowExtent.height;

	return {
	    std::clamp<uint32_t>(width,
	                         capabilities.minImageExtent.width,
	                         capabilities.maxImageExtent.width),
	    std::clamp<uint32_t>(height,
	                         capabilities.minImageExtent.height,
	                         capabilities.maxImageExtent.height),
	};
}

uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities)
{
	auto minImageCount = std::max(3u, capabilities.minImageCount);
	if ((capabilities.maxImageCount > 0) &&
	    (minImageCount > capabilities.maxImageCount)) {
		return capabilities.maxImageCount;
	}

	return minImageCount;
}
}        // namespace

// Constructor & Destructor

SwapChain::SwapChain(VulkanDevice &device, vk::Extent2D windowExtent) :
    device{device}, windowExtent{windowExtent}
{
	createSwapChain();
	createImageViews();
	createDepthResources();
	createSyncObjects();
}

// Public Methods

std::pair<vk::Result, uint32_t> SwapChain::acquireNextImage(uint32_t frameIndex)
{
	auto fenceResult = device.device().waitForFences(
	    *inFlightFences[frameIndex], vk::True, UINT64_MAX);
	if (fenceResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to wait for fence!");
	}

	auto [result, imageIndex] = swapChain.acquireNextImage(
	    UINT64_MAX, *imageAvailableSemaphores[frameIndex], nullptr);

	return {result, imageIndex};
}

vk::Result SwapChain::submitCommandBuffers(
    const vk::raii::CommandBuffer &commandBuffer,
    uint32_t                       imageIndex,
    uint32_t                       frameIndex)
{
	vk::PipelineStageFlags waitDestinationStageMask(
	    vk::PipelineStageFlagBits::eColorAttachmentOutput);
	const vk::SubmitInfo submitInfo{
	    .waitSemaphoreCount   = 1,
	    .pWaitSemaphores      = &*imageAvailableSemaphores[frameIndex],
	    .pWaitDstStageMask    = &waitDestinationStageMask,
	    .commandBufferCount   = 1,
	    .pCommandBuffers      = &*commandBuffer,
	    .signalSemaphoreCount = 1,
	    .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex],
	};

	device.graphicsQueue().submit(submitInfo, *inFlightFences[frameIndex]);

	const vk::PresentInfoKHR presentInfoKHR{
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
	    .swapchainCount     = 1,
	    .pSwapchains        = &*swapChain,
	    .pImageIndices      = &imageIndex,
	};

	return device.presentQueue().presentKHR(presentInfoKHR);
}

vk::Format SwapChain::findDepthFormat()
{
	return device.findSupportedFormat(
	    {vk::Format::eD32Sfloat,
	     vk::Format::eD32SfloatS8Uint,
	     vk::Format::eD24UnormS8Uint},
	    vk::ImageTiling::eOptimal,
	    vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

// Private Init Methods

void SwapChain::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

	swapChainSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode =
	    chooseSwapPresentMode(swapChainSupport.presentModes);
	swapChainExtent =
	    chooseSwapExtent(swapChainSupport.capabilities, windowExtent);
	uint32_t imageCount =
	    chooseSwapMinImageCount(swapChainSupport.capabilities);

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

	QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
	uint32_t           queueFamilyIndices[] = {indices.graphicsFamily.value(),
	                                           indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily) {
		swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
	} else {
		swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eExclusive;
		swapChainCreateInfo.queueFamilyIndexCount = 0;              // Optional
		swapChainCreateInfo.pQueueFamilyIndices   = nullptr;        // Optional
	}

	swapChain = vk::raii::SwapchainKHR(device.device(), swapChainCreateInfo);
	swapChainImages = swapChain.getImages();
}

void SwapChain::createImageViews()
{
	assert(swapChainImageViews.empty());

	for (auto &image : swapChainImages) {
		swapChainImageViews.push_back(
		    device.createImageView(image,
		                           swapChainSurfaceFormat.format,
		                           vk::ImageAspectFlagBits::eColor));
	}
}

void SwapChain::createDepthResources()
{
	vk::Format   depthFormat     = findDepthFormat();
	vk::Extent2D swapChainExtent = getSwapChainExtent();

	std::tie(depthImage, depthImageMemory) =
	    device.createImage(swapChainExtent.width,
	                       swapChainExtent.height,
	                       depthFormat,
	                       vk::ImageTiling::eOptimal,
	                       vk::ImageUsageFlagBits::eDepthStencilAttachment,
	                       vk::MemoryPropertyFlagBits::eDeviceLocal);
	depthImageView = device.createImageView(
	    *depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

void SwapChain::createSyncObjects()
{
	assert(renderFinishedSemaphores.empty() &&
	       imageAvailableSemaphores.empty() && inFlightFences.empty());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		renderFinishedSemaphores.emplace_back(device.device(),
		                                      vk::SemaphoreCreateInfo{});
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		imageAvailableSemaphores.emplace_back(device.device(),
		                                      vk::SemaphoreCreateInfo{});
		inFlightFences.emplace_back(
		    device.device(),
		    vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
	}
}

}        // namespace mvr
