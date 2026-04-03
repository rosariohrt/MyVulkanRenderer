#include "vulkan_device.h"

// std
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <string_view>
#include <vector>

namespace mvr
{

// ==================================================
// Public Functions
// ==================================================

VulkanDevice::VulkanDevice(Window &window) :
    window{window}
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createCommandPool();
}

VulkanDevice::~VulkanDevice()
{
	vkDestroyCommandPool(device_, commandPool, nullptr);
	vkDestroyDevice(device_, nullptr);

	vkDestroySurfaceKHR(*instance, surface_, nullptr);
}

// ==================================================
// Utilities
// ==================================================

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &memProperties);
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

// ==================================================
// Buffer Helper Functions
// ==================================================

void VulkanDevice::createBuffer(
    VkDeviceSize          size,
    VkBufferUsageFlags    usage,
    VkMemoryPropertyFlags properties,
    VkBuffer             &buffer,
    VkDeviceMemory       &bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanDevice::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

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

	vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue_);

	vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
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
    VkMemoryPropertyFlags    properties,
    VkImage                 &image,
    VkDeviceMemory          &imageMemory)
{
	if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device_, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	if (vkBindImageMemory(device_, image, imageMemory, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind image memory!");
	}
}

// ==================================================
// Private Setup Functions
// ==================================================

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
	window.createWindowSurface(*instance, &surface_);
}

void VulkanDevice::pickPhysicalDevice()
{
	auto physicalDevices = instance.enumeratePhysicalDevices();
	if (physicalDevices.empty()) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	// Log all available physical devices
	std::cout << "Device count: " << physicalDevices.size() << std::endl;
	for (const auto &device : physicalDevices) {
		auto props = device.getProperties();
		std::cout << "Physical device: " << props.deviceName
		          << " (Type: " << to_string(props.deviceType) << ")" << std::endl;
	}

	// Find the first suitable device
	auto const it = std::ranges::find_if(physicalDevices, [&](auto const &physicalDevice) {
		return isDeviceSuitable(physicalDevice);
	});
	if (it == physicalDevices.end()) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	// TODO: Implement a scoring system to select the most suitable GPU.
	// Currently, it picks the first one that meets the minimum requirements.
	// A more robust approach would evaluate deviceType (Discrete > Integrated),
	// VRAM capacity, and limits.maxImageDimension2D to assign a quality score.

	physicalDevice = *it;
}

void VulkanDevice::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(*physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t>                   uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex        = queueFamily;
		queueCreateInfo.queueCount              = 1;
		queueCreateInfo.pQueuePriorities        = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy        = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos    = queueCreateInfos.data();

	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// might not really be necessary anymore because device specific validation layers
	// have been deprecated
	if (enableValidationLayers) {
		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(*physicalDevice, &createInfo, nullptr, &device_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
	vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
}

void VulkanDevice::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex        = queueFamilyIndices.graphicsFamily;
	poolInfo.flags =
	    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

// ==================================================
// Private Helper Functions
// ==================================================

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
	bool supportsVulkan1_3   = hasRequiredApiVersion(physicalDevice);
	bool supportGraphics     = hasGraphicsSupport(physicalDevice);
	bool extensionsSupported = hasRequiredExtensions(physicalDevice);
	bool featuresSupported   = hasRequiredFeatures(physicalDevice);
	bool swapChainAdequate   = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(*physicalDevice);
		swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return supportsVulkan1_3 && supportGraphics && extensionsSupported && featuresSupported && swapChainAdequate;
}

bool VulkanDevice::hasRequiredApiVersion(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if the physicalDevice supports the Vulkan 1.3 API version
	return physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;
}

bool VulkanDevice::hasGraphicsSupport(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if any of the queue families support graphics operations
	auto queueFamilies = physicalDevice.getQueueFamilyProperties();

	return std::ranges::any_of(queueFamilies, [](auto const &qfp) {
		return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
	});
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

	return std::ranges::all_of(deviceExtensions, isSupported);
}

bool VulkanDevice::hasRequiredFeatures(vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
	auto features = physicalDevice.template getFeatures2<
	    vk::PhysicalDeviceFeatures2,
	    vk::PhysicalDeviceVulkan13Features,
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	return features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
	       features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
		    device,
		    surface_,
		    &presentModeCount,
		    details.presentModes.data());
	}
	return details;
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily         = i;
			indices.graphicsFamilyHasValue = true;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily         = i;
			indices.presentFamilyHasValue = true;
		}
		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

}        // namespace mvr