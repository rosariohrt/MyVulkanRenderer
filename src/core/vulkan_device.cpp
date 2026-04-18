#include "vulkan_device.h"
#include "vulkan/vulkan.hpp"

// std
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace mvr
{

// Constructor & Destructor

VulkanDevice::VulkanDevice(Window &window) :
    window{window}
{
	try {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createCommandPool();
	} catch (const std::exception &e) {
		std::cerr << "VulkanDevice initialization aborted: " << e.what() << std::endl;
		throw;
	}
}

// Public Methods

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkFormat VulkanDevice::findSupportedFormat(
    const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(*physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (
		    tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

VkCommandBuffer VulkanDevice::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = *commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(*device_, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(*graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(*graphicsQueue_);

	vkFreeCommandBuffers(*device_, *commandPool, 1, &commandBuffer);
}

void VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;        // Optional
	copyRegion.dstOffset = 0;        // Optional
	copyRegion.size      = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void VulkanDevice::copyBufferToImage(
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset      = 0;
	region.bufferRowLength   = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = layerCount;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(
	    commandBuffer,
	    buffer,
	    image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    1,
	    &region);
	endSingleTimeCommands(commandBuffer);
}

void VulkanDevice::createImageWithInfo(
    const VkImageCreateInfo &imageInfo,
    vk::MemoryPropertyFlags  properties,
    VkImage                 &image,
    VkDeviceMemory          &imageMemory)
{
	if (vkCreateImage(*device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(*device_, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(*device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	if (vkBindImageMemory(*device_, image, imageMemory, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind image memory!");
	}
}

// Private Init Methods

void VulkanDevice::createInstance()
{
	constexpr vk::ApplicationInfo appInfo{
	    .pApplicationName   = "MyVulkanRenderer App",
	    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
	    .pEngineName        = "No Engine",
	    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
	    .apiVersion         = vk::ApiVersion14};

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) {
		populateDebugMessengerCreateInfo(debugCreateInfo);
	}

	vk::InstanceCreateFlags flags = {};
#if __APPLE__
	flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

	auto layers     = getRequiredLayers();
	auto extensions = getRequiredExtensions();

	vk::InstanceCreateInfo createInfo = {
	    .pNext                   = enableValidationLayers ? &debugCreateInfo : nullptr,
	    .flags                   = flags,
	    .pApplicationInfo        = &appInfo,
	    .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
	    .ppEnabledLayerNames     = layers.empty() ? nullptr : layers.data(),
	    .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
	    .ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data(),
	};

	instance = vk::raii::Instance(context, createInfo);
}

void VulkanDevice::setupDebugMessenger()
{
	if (!enableValidationLayers) {
		return;
	}

	vk::DebugUtilsMessengerCreateInfoEXT debugInfo;
	populateDebugMessengerCreateInfo(debugInfo);
	debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
}

void VulkanDevice::createSurface()
{
	VkSurfaceKHR _surface;
	window.createWindowSurface(*instance, &_surface);
	surface_ = vk::raii::SurfaceKHR(instance, _surface);
}

void VulkanDevice::pickPhysicalDevice()
{
	auto physicalDevices = instance.enumeratePhysicalDevices();
	if (physicalDevices.empty()) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	// Log all available physical devices and find the first suitable one
	std::cout << "Device count: " << physicalDevices.size() << std::endl;
	std::cout << "Available GPUs:" << std::endl;
	bool found = false;
	for (const auto &candidate : physicalDevices) {
		auto props = candidate.getProperties();
		std::cout << "\t" << props.deviceName << " (Type: " << to_string(props.deviceType) << ")" << std::endl;

		if (!found && isDeviceSuitable(candidate)) {
			physicalDevice     = candidate;
			queueFamilyIndices = findQueueFamilies(physicalDevice);
			found              = true;
		}
	}

	if (!found) {
		throw std::runtime_error("Failed to find a suitable GPU!");
	}

	std::cout << "Selected GPU: " << physicalDevice.getProperties().deviceName << std::endl;

	// TODO: Implement a scoring system to select the most suitable GPU.
	// Currently, it picks the first one that meets the minimum requirements.
	// A more robust approach would evaluate deviceType (Discrete > Integrated),
	// VRAM capacity, and limits.maxImageDimension2D to assign a quality score.
}

void VulkanDevice::createLogicalDevice()
{
	// Create queue create infos for each unique queue family
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t>                     uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo = {
		    .queueFamilyIndex = queueFamily,
		    .queueCount       = 1,
		    .pQueuePriorities = &queuePriority};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Enable required features using StructureChain
	vk::StructureChain<
	    vk::PhysicalDeviceFeatures2,
	    vk::PhysicalDeviceVulkan13Features,
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
	    featureChain = {
	        {},                                   // vk::PhysicalDeviceFeatures2 (empty for now)
	        {.dynamicRendering = true},           // Enable dynamic rendering from Vulkan 1.3
	        {.extendedDynamicState = true}        // Enable extended dynamic state from the extension
	    };

	vk::DeviceCreateInfo deviceCreateInfo = {
	    .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
	    .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
	    .pQueueCreateInfos       = queueCreateInfos.data(),
	    .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
	    .ppEnabledExtensionNames = deviceExtensions.data(),
	};

	device_ = vk::raii::Device(physicalDevice, deviceCreateInfo);

	graphicsQueue_ = vk::raii::Queue(device_, queueFamilyIndices.graphicsFamily.value(), 0);
	presentQueue_  = vk::raii::Queue(device_, queueFamilyIndices.presentFamily.value(), 0);
}

void VulkanDevice::createCommandPool()
{
	vk::CommandPoolCreateInfo poolInfo = {
	    .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
	};

	commandPool = vk::raii::CommandPool(device_, poolInfo);
}

// Helper Methods

void VulkanDevice::populateDebugMessengerCreateInfo(
    vk::DebugUtilsMessengerCreateInfoEXT &debugInfo)
{
	debugInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
	                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
	debugInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
	                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
	                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
	debugInfo.pfnUserCallback = debugCallback;
	debugInfo.pUserData       = nullptr;        // Optional
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanDevice::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
    vk::DebugUtilsMessageTypeFlagsEXT             type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                         *pUserData)
{
	std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

	return vk::False;
}

std::vector<const char *> VulkanDevice::getRequiredLayers()
{
	if (!enableValidationLayers) {
		return {};
	}

	// Get the required layers
	std::vector<char const *> requiredLayers;
	requiredLayers.assign(validationLayers.begin(), validationLayers.end());

	// Log all available instance layers supported by the system
	auto supportedLayers = context.enumerateInstanceLayerProperties();
	std::cout << "Supported layers:" << std::endl;
	for (const auto &prop : supportedLayers) {
		std::cout << "\t" << prop.layerName << std::endl;
	}

	// Log the layers requested for this instance
	std::cout << "Required layers:" << std::endl;
	for (const auto &req : requiredLayers) {
		std::cout << "\t" << req << std::endl;
	}

	// Check if the required layers are supported by the Vulkan implementation.
	auto it = std::ranges::find_if(requiredLayers, [&](auto const &req) {
		return std::ranges::none_of(supportedLayers, [&](auto const &prop) {
			return std::string_view(prop.layerName) == req;
		});
	});

	if (it != requiredLayers.end()) {
		throw std::runtime_error("Required validation layer not supported: " + std::string(*it));
	}

	return requiredLayers;
}

std::vector<const char *> VulkanDevice::getRequiredExtensions()
{
	// Get the required extensions
	uint32_t glfwExtensionCount = 0;
	auto     glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		requiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
	}
#if __APPLE__
	requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

	// Log all available instance extensions supported by the system
	auto supportedExtensions = context.enumerateInstanceExtensionProperties();
	std::cout << "Supported extensions:" << std::endl;
	for (const auto &prop : supportedExtensions) {
		std::cout << "\t" << prop.extensionName << std::endl;
	}

	// Log the extensions requested for this instance
	std::cout << "Required extensions:" << std::endl;
	for (const auto &req : requiredExtensions) {
		std::cout << "\t" << req << std::endl;
	}

	// Check if the required extensions are supported by the Vulkan implementation.
	auto it = std::ranges::find_if(requiredExtensions, [&](auto const &req) {
		return std::ranges::none_of(supportedExtensions, [&](auto const &prop) {
			return std::string_view(prop.extensionName) == req;
		});
	});

	if (it != requiredExtensions.end()) {
		throw std::runtime_error("Required extension not supported: " + std::string(*it));
	}

	return requiredExtensions;
}

bool VulkanDevice::isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice)
{
	return hasRequiredApiVersion(physicalDevice) &&
	       hasGraphicsSupport(physicalDevice) &&
	       hasRequiredExtensions(physicalDevice) &&
	       hasRequiredFeatures(physicalDevice) &&
	       hasSwapchainSupport(physicalDevice);
}

bool VulkanDevice::hasRequiredApiVersion(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if the physicalDevice supports the Vulkan 1.3 API version
	bool result = physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;
	if (!result) {
		std::cerr << "\t" << " - Does not support Vulkan 1.3 API version" << std::endl;
	}

	return result;
}

bool VulkanDevice::hasGraphicsSupport(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if any of the queue families support graphics operations
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	bool               result  = indices.isComplete();
	if (!result) {
		std::cerr << "\t" << " - Missing required queue families" << std::endl;
	}

	return result;
}

bool VulkanDevice::hasRequiredExtensions(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if all required physicalDevice extensions are available
	auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

	auto isSupported = [&](const char *extensionName) {
		return std::ranges::any_of(availableDeviceExtensions, [&](auto const &prop) {
			return std::string_view(prop.extensionName) == extensionName;
		});
	};

	bool result = std::ranges::all_of(deviceExtensions, isSupported);
	if (!result) {
		std::cerr << "\t" << " - Missing required extensions" << std::endl;
	}

	return result;
}

bool VulkanDevice::hasRequiredFeatures(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
	auto features = physicalDevice.template getFeatures2<
	    vk::PhysicalDeviceFeatures2,
	    vk::PhysicalDeviceVulkan13Features,
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	bool result = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
	              features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
	if (!result) {
		std::cerr << "\t" << " - Does not support required features" << std::endl;
	}

	return result;
}

bool VulkanDevice::hasSwapchainSupport(vk::raii::PhysicalDevice const &physicalDevice) const
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	bool                    result           = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	if (!result) {
		std::cerr << "\t" << " - Does not support required swapchain support" << std::endl;
	}

	return result;
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(vk::raii::PhysicalDevice const &physicalDevice) const
{
	QueueFamilyIndices indices;

	// Get all queue families
	std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

	// Find queue families that support graphics and present.
	for (uint32_t i = 0; i < queueFamilies.size(); i++) {
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		if (physicalDevice.getSurfaceSupportKHR(i, surface_)) {
			indices.presentFamily = i;
		}
	}

	return indices;
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(vk::raii::PhysicalDevice const &physicalDevice) const
{
	SwapChainSupportDetails details;
	details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface_);
	details.formats      = physicalDevice.getSurfaceFormatsKHR(*surface_);
	details.presentModes = physicalDevice.getSurfacePresentModesKHR(*surface_);

	return details;
}

}        // namespace mvr