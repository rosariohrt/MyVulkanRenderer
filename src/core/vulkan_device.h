#pragma once

#include "window.h"

// std
#include <vector>

namespace mvr
{

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR        capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>   presentModes;
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
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	VkPhysicalDeviceProperties properties;

	// Constructor & Destructor
	VulkanDevice(Window &window);
	~VulkanDevice();

	// Not copyable or movable
	VulkanDevice(const VulkanDevice &)       = delete;
	void operator=(const VulkanDevice &)     = delete;
	VulkanDevice(VulkanDevice &&)            = delete;
	VulkanDevice &operator=(VulkanDevice &&) = delete;

	// Getters  (inline)
	VkCommandPool getCommandPool()
	{
		return commandPool;
	}
	VkDevice device()
	{
		return *device_;
	}
	VkSurfaceKHR surface()
	{
		return surface_;
	}
	VkQueue graphicsQueue()
	{
		return *graphicsQueue_;
	}
	VkQueue presentQueue()
	{
		return *presentQueue_;
	}
	SwapChainSupportDetails getSwapChainSupport()
	{
		return querySwapChainSupport(*physicalDevice);
	}
	QueueFamilyIndices findPhysicalQueueFamilies()
	{
		return findQueueFamilies(physicalDevice);
	}

	// Utilities
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(
	    const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// Buffer Helper Functions
	void createBuffer(
	    VkDeviceSize          size,
	    VkBufferUsageFlags    usage,
	    VkMemoryPropertyFlags properties,
	    VkBuffer             &buffer,
	    VkDeviceMemory       &bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void            endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void            copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void            copyBufferToImage(
	               VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

	void createImageWithInfo(
	    const VkImageCreateInfo &imageInfo,
	    VkMemoryPropertyFlags    properties,
	    VkImage                 &image,
	    VkDeviceMemory          &imageMemory);

  private:
	vk::raii::Context                context;
	vk::raii::Instance               instance       = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	vk::raii::PhysicalDevice         physicalDevice = nullptr;
	Window                          &window;
	VkCommandPool                    commandPool;

	vk::raii::Device device_ = nullptr;
	VkSurfaceKHR     surface_;

	vk::raii::Queue graphicsQueue_ = nullptr;
	vk::raii::Queue presentQueue_  = nullptr;

	// queue families indices
	QueueFamilyIndices queueFamilyIndices;

	const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef __APPLE__
	const std::vector<const char *> deviceExtensions = {vk::KHRSwapchainExtensionName, vk::KHRPortabilitySubsetExtensionName};
#else
	const std::vector<const char *> deviceExtensions = {vk::KHRSwapchainExtensionName};
#endif

	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	// helper functions
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
	QueueFamilyIndices        findQueueFamilies(vk::raii::PhysicalDevice const &physicalDevice);
	SwapChainSupportDetails   querySwapChainSupport(VkPhysicalDevice device);
};

}        // namespace mvr