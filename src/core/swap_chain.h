#pragma once

#include "vulkan_device.h"

// std
#include <utility>
#include <vector>

namespace mvr
{

class SwapChain
{
  public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	// Constructor & Destructor
	SwapChain(VulkanDevice &deviceRef, VkExtent2D windowExtent);
	~SwapChain();

	SwapChain(const SwapChain &)      = delete;
	void operator=(const SwapChain &) = delete;

	// Public Methods
	std::pair<vk::Result, uint32_t> acquireNextImage(uint32_t frameIndex);
	vk::Result                      submitCommandBuffers(const vk::raii::CommandBuffer &commandBuffer, uint32_t imageIndex, uint32_t frameIndex);
	VkFormat                        findDepthFormat();

	vk::Image getImage(uint32_t index)
	{
		return swapChainImages[index];
	}
	vk::raii::ImageView &getImageView(uint32_t index)
	{
		return swapChainImageViews[index];
	}
	size_t imageCount()
	{
		return swapChainImages.size();
	}
	vk::Format getSwapChainSurfaceFormat()
	{
		return swapChainSurfaceFormat.format;
	}
	vk::Extent2D getSwapChainExtent()
	{
		return swapChainExtent;
	}
	float extentAspectRatio()
	{
		return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
	}
	vk::Viewport getViewport()
	{
		return vk::Viewport{
		    .x        = 0.0f,
		    .y        = 0.0f,
		    .width    = static_cast<float>(swapChainExtent.width),
		    .height   = static_cast<float>(swapChainExtent.height),
		    .minDepth = 0.0f,
		    .maxDepth = 1.0f,
		};
	}
	vk::Rect2D getScissor()
	{
		return vk::Rect2D{
		    .offset = {0, 0},
		    .extent = swapChainExtent,
		};
	}

  private:
	// Variables
	VulkanDevice &device;
	VkExtent2D    windowExtent;

	vk::SurfaceFormatKHR swapChainSurfaceFormat;
	vk::Extent2D         swapChainExtent;

	vk::raii::SwapchainKHR swapChain = nullptr;

	std::vector<vk::Image>           swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	std::vector<VkImage>        depthImages;
	std::vector<VkDeviceMemory> depthImageMemorys;
	std::vector<VkImageView>    depthImageViews;

	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
	std::vector<vk::raii::Fence>     inFlightFences;

	// Private Init Methods
	void createSwapChain();
	void createImageViews();
	void createDepthResources();
	void createSyncObjects();

	// Helper Methods
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
	vk::PresentModeKHR   chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
	vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
	uint32_t             chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities);
};

}        // namespace mvr
