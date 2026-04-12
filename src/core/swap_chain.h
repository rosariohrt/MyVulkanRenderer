#pragma once

#include "vulkan_device.h"

// std
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
	VkResult acquireNextImage(uint32_t *imageIndex);
	VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);
	VkFormat findDepthFormat();

	// Getters
	VkFramebuffer getFrameBuffer(int index)
	{
		return swapChainFramebuffers[index];
	}
	VkRenderPass getRenderPass()
	{
		return renderPass;
	}
	VkImageView getImageView(int index)
	{
		return *swapChainImageViews[index];
	}
	size_t imageCount()
	{
		return swapChainImages.size();
	}
	VkFormat getSwapChainSurfaceFormat()
	{
		return static_cast<VkFormat>(swapChainSurfaceFormat.format);
	}
	VkExtent2D getSwapChainExtent()
	{
		return swapChainExtent;
	}
	uint32_t width()
	{
		return swapChainExtent.width;
	}
	uint32_t height()
	{
		return swapChainExtent.height;
	}
	float extentAspectRatio()
	{
		return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
	}

  private:
	// Variables
	VulkanDevice &device;
	VkExtent2D    windowExtent;

	vk::SurfaceFormatKHR swapChainSurfaceFormat;
	vk::Extent2D         swapChainExtent;
	size_t               currentFrame = 0;

	vk::raii::SwapchainKHR swapChain = nullptr;

	std::vector<vk::Image>           swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	VkRenderPass renderPass;

	std::vector<VkImage>        depthImages;
	std::vector<VkDeviceMemory> depthImageMemorys;
	std::vector<VkImageView>    depthImageViews;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence>     inFlightFences;
	std::vector<VkFence>     imagesInFlight;

	// Private Init Methods
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDepthResources();
	void createFramebuffers();
	void createSyncObjects();

	// Helper Methods
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
	vk::PresentModeKHR   chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
	vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
	uint32_t             chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities);
};

}        // namespace mvr
