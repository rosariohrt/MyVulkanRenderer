#pragma once

#include "window.h"

// std
#include <optional>
#include <vector>

namespace mvr
{

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR        capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR>   presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	// TODO: std::optional<uint32_t> computeFamily;
	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class VulkanDevice
{
  public:
	// Variables
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	VkPhysicalDeviceProperties properties;

	// Constructor & Destructor
	VulkanDevice(Window &window);

	// Not copyable or movable
	VulkanDevice(const VulkanDevice &)       = delete;
	void operator=(const VulkanDevice &)     = delete;
	VulkanDevice(VulkanDevice &&)            = delete;
	VulkanDevice &operator=(VulkanDevice &&) = delete;

	// Public Methods
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkCommandBuffer beginSingleTimeCommands();
	void            endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void            copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void            copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
	void            createImageWithInfo(const VkImageCreateInfo &imageInfo, vk::MemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

	// Getters
	vk::raii::CommandPool &getCommandPool()
	{
		return commandPool;
	}
	vk::raii::Device &device()
	{
		return device_;
	}
	Window &getWindow()
	{
		return window;
	}
	VkSurfaceKHR surface()
	{
		return *surface_;
	}
	vk::raii::Queue &graphicsQueue()
	{
		return graphicsQueue_;
	}
	vk::raii::Queue &presentQueue()
	{
		return presentQueue_;
	}
	SwapChainSupportDetails getSwapChainSupport()
	{
		return querySwapChainSupport(physicalDevice);
	}
	QueueFamilyIndices findPhysicalQueueFamilies()
	{
		return findQueueFamilies(physicalDevice);
	}

  private:
	// Variables
	Window &window;

	const std::vector<const char *> validationLayers = {
	    "VK_LAYER_KHRONOS_validation",
	    // "VK_LAYER_LUNARG_crash_diagnostic",
	};

#ifdef __APPLE__
	const std::vector<const char *> deviceExtensions = {vk::KHRSwapchainExtensionName, "VK_KHR_portability_subset"};
#else
	const std::vector<const char *> deviceExtensions = {vk::KHRSwapchainExtensionName};
#endif

	QueueFamilyIndices queueFamilyIndices;

	vk::raii::Context                context;
	vk::raii::Instance               instance       = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	vk::raii::SurfaceKHR             surface_       = nullptr;
	vk::raii::PhysicalDevice         physicalDevice = nullptr;
	vk::raii::Device                 device_        = nullptr;

	vk::raii::Queue graphicsQueue_ = nullptr;
	vk::raii::Queue presentQueue_  = nullptr;

	vk::raii::CommandPool commandPool = nullptr;

	// Private Init Methods
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	// Helper Methods
	void              populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT &createInfo);
	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
	    vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	    vk::DebugUtilsMessageTypeFlagsEXT             messageType,
	    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
	    void                                         *pUserData);
	std::vector<const char *> getRequiredLayers();
	std::vector<const char *> getRequiredExtensions();
	bool                      isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice);
	bool                      hasRequiredApiVersion(vk::raii::PhysicalDevice const &physicalDevice) const;
	bool                      hasGraphicsSupport(vk::raii::PhysicalDevice const &physicalDevice) const;
	bool                      hasRequiredExtensions(vk::raii::PhysicalDevice const &physicalDevice) const;
	bool                      hasRequiredFeatures(vk::raii::PhysicalDevice const &physicalDevice) const;
	bool                      hasSwapchainSupport(vk::raii::PhysicalDevice const &physicalDevice) const;
	QueueFamilyIndices        findQueueFamilies(vk::raii::PhysicalDevice const &physicalDevice) const;
	SwapChainSupportDetails   querySwapChainSupport(vk::raii::PhysicalDevice const &physicalDevice) const;
};

}        // namespace mvr