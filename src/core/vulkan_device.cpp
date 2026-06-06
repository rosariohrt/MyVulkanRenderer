#include "vulkan_device.h"

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

VulkanDevice::VulkanDevice(Window &window) : window{window}
{
	try {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createCommandPool();
	} catch (const std::exception &e) {
		std::cerr << "VulkanDevice initialization aborted: " << e.what()
		          << std::endl;
		throw;
	}
}

// Public Methods

uint32_t VulkanDevice::findMemoryType(uint32_t                typeFilter,
                                      vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties =
	    physicalDevice_.getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) ==
		        properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

vk::Format
    VulkanDevice::findSupportedFormat(const std::vector<vk::Format> &candidates,
                                      vk::ImageTiling                tiling,
                                      vk::FormatFeatureFlags         features)
{
	for (const auto format : candidates) {
		vk::FormatProperties props =
		    physicalDevice_.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear &&
		    (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal &&
		           (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    VulkanDevice::createBuffer(vk::DeviceSize          size,
                               vk::BufferUsageFlags    usage,
                               vk::MemoryPropertyFlags properties)
{
	vk::BufferCreateInfo bufferInfo{
	    .size        = size,
	    .usage       = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};

	vk::raii::Buffer buffer(device_, bufferInfo);

	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo{
	    .allocationSize = memRequirements.size,
	    .memoryTypeIndex =
	        findMemoryType(memRequirements.memoryTypeBits, properties),
	};
	vk::raii::DeviceMemory bufferMemory(device_, allocInfo);

	buffer.bindMemory(*bufferMemory, 0);

	return {std::move(buffer), std::move(bufferMemory)};
}

vk::raii::CommandBuffer VulkanDevice::beginSingleTimeCommands()
{
	vk::CommandBufferAllocateInfo allocInfo = {
	    .commandPool        = commandPool,
	    .level              = vk::CommandBufferLevel::ePrimary,
	    .commandBufferCount = 1,
	};
	vk::raii::CommandBuffer commandBuffer =
	    std::move(vk::raii::CommandBuffers(device_, allocInfo).front());

	vk::CommandBufferBeginInfo beginInfo = {
	    .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};
	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(
    vk::raii::CommandBuffer &&commandBuffer)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo = {
	    .commandBufferCount = 1,
	    .pCommandBuffers    = &*commandBuffer,
	};
	graphicsQueue_.submit(submitInfo, nullptr);
	graphicsQueue_.waitIdle();
}

void VulkanDevice::copyBuffer(vk::raii::Buffer &srcBuffer,
                              vk::raii::Buffer &dstBuffer,
                              vk::DeviceSize    size)
{
	vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
	commandCopyBuffer.copyBuffer(
	    *srcBuffer, *dstBuffer, vk::BufferCopy{.size = size});
	endSingleTimeCommands(std::move(commandCopyBuffer));
}

void VulkanDevice::copyBufferToImage(vk::raii::CommandBuffer &commandBuffer,
                                     const vk::raii::Buffer  &buffer,
                                     vk::raii::Image         &image,
                                     uint32_t                 width,
                                     uint32_t                 height)
{
	vk::BufferImageCopy region = {
	    .bufferOffset      = 0,
	    .bufferRowLength   = 0,
	    .bufferImageHeight = 0,
	    .imageSubresource =
	        {
	            .aspectMask     = vk::ImageAspectFlagBits::eColor,
	            .mipLevel       = 0,
	            .baseArrayLayer = 0,
	            .layerCount     = 1,
	        },
	    .imageOffset = {0, 0, 0},
	    .imageExtent = {width, height, 1},
	};
	commandBuffer.copyBufferToImage(
	    buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory>
    VulkanDevice::createImage(uint32_t                width,
                              uint32_t                height,
                              vk::Format              format,
                              vk::ImageTiling         tiling,
                              vk::ImageUsageFlags     usage,
                              vk::MemoryPropertyFlags properties)
{
	vk::ImageCreateInfo imageInfo = {
	    .imageType   = vk::ImageType::e2D,
	    .format      = format,
	    .extent      = {width, height, 1},
	    .mipLevels   = 1,
	    .arrayLayers = 1,
	    .samples     = vk::SampleCountFlagBits::e1,
	    .tiling      = tiling,
	    .usage       = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};

	vk::raii::Image image = vk::raii::Image(device_, imageInfo);

	vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo       = {
	    .allocationSize = memRequirements.size,
	    .memoryTypeIndex =
	        findMemoryType(memRequirements.memoryTypeBits, properties),
	};

	vk::raii::DeviceMemory imageMemory =
	    vk::raii::DeviceMemory(device_, allocInfo);
	image.bindMemory(*imageMemory, 0);

	return {std::move(image), std::move(imageMemory)};
}

vk::raii::ImageView VulkanDevice::createImageView(
    vk::raii::Image &image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	vk::ImageViewCreateInfo viewInfo = {
	    .image    = image,
	    .viewType = vk::ImageViewType::e2D,
	    .format   = format,
	    .subresourceRange{
	        .aspectMask     = aspectFlags,
	        .baseMipLevel   = 0,
	        .levelCount     = 1,
	        .baseArrayLayer = 0,
	        .layerCount     = 1,
	    },
	};

	return vk::raii::ImageView(device_, viewInfo);
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
	    .pNext            = enableValidationLayers ? &debugCreateInfo : nullptr,
	    .flags            = flags,
	    .pApplicationInfo = &appInfo,
	    .enabledLayerCount     = static_cast<uint32_t>(layers.size()),
	    .ppEnabledLayerNames   = layers.empty() ? nullptr : layers.data(),
	    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	    .ppEnabledExtensionNames =
	        extensions.empty() ? nullptr : extensions.data(),
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
		std::cout << "\t" << props.deviceName
		          << " (Type: " << to_string(props.deviceType) << ")"
		          << std::endl;

		if (!found && isDeviceSuitable(candidate)) {
			physicalDevice_    = candidate;
			queueFamilyIndices = findQueueFamilies(physicalDevice_);
			found              = true;
		}
	}

	if (!found) {
		throw std::runtime_error("Failed to find a suitable GPU!");
	}

	std::cout << "Selected GPU: " << physicalDevice_.getProperties().deviceName
	          << std::endl;

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
	vk::StructureChain<vk::PhysicalDeviceFeatures2,
	                   vk::PhysicalDeviceVulkan13Features,
	                   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
	    featureChain = {
	        {.features = {.samplerAnisotropy =
	                          true}},        // vk::PhysicalDeviceFeatures2
	        {.synchronization2 = true,
	         .dynamicRendering =
	             true},        // Enable sync2 and dynamic rendering
	        {.extendedDynamicState = true}        // Enable extended dynamic
	                                              // state from the extension
	    };

	vk::DeviceCreateInfo deviceCreateInfo = {
	    .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
	    .queueCreateInfoCount  = static_cast<uint32_t>(queueCreateInfos.size()),
	    .pQueueCreateInfos     = queueCreateInfos.data(),
	    .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
	    .ppEnabledExtensionNames = deviceExtensions.data(),
	};

	device_ = vk::raii::Device(physicalDevice_, deviceCreateInfo);

	graphicsQueue_ =
	    vk::raii::Queue(device_, queueFamilyIndices.graphicsFamily.value(), 0);
	presentQueue_ =
	    vk::raii::Queue(device_, queueFamilyIndices.presentFamily.value(), 0);
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
	debugInfo.messageSeverity =
	    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
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
	std::cerr << "validation layer: type " << to_string(type)
	          << " msg: " << pCallbackData->pMessage << std::endl;

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
		throw std::runtime_error("Required validation layer not supported: " +
		                         std::string(*it));
	}

	return requiredLayers;
}

std::vector<const char *> VulkanDevice::getRequiredExtensions()
{
	// Get the required extensions
	uint32_t glfwExtensionCount = 0;
	auto     glfwExtensions =
	    glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> requiredExtensions(
	    glfwExtensions, glfwExtensions + glfwExtensionCount);
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

	// Check if the required extensions are supported by the Vulkan
	// implementation.
	auto it = std::ranges::find_if(requiredExtensions, [&](auto const &req) {
		return std::ranges::none_of(supportedExtensions, [&](auto const &prop) {
			return std::string_view(prop.extensionName) == req;
		});
	});

	if (it != requiredExtensions.end()) {
		throw std::runtime_error("Required extension not supported: " +
		                         std::string(*it));
	}

	return requiredExtensions;
}

bool VulkanDevice::isDeviceSuitable(
    vk::raii::PhysicalDevice const &physicalDevice)
{
	return hasRequiredApiVersion(physicalDevice) &&
	       hasGraphicsSupport(physicalDevice) &&
	       hasRequiredExtensions(physicalDevice) &&
	       hasRequiredFeatures(physicalDevice) &&
	       hasSwapchainSupport(physicalDevice);
}

bool VulkanDevice::hasRequiredApiVersion(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if the physicalDevice supports the Vulkan 1.3 API version
	bool result =
	    physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;
	if (!result) {
		std::cerr << "\t" << " - Does not support Vulkan 1.3 API version"
		          << std::endl;
	}

	return result;
}

bool VulkanDevice::hasGraphicsSupport(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if any of the queue families support graphics operations
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	bool               result  = indices.isComplete();
	if (!result) {
		std::cerr << "\t" << " - Missing required queue families" << std::endl;
	}

	return result;
}

bool VulkanDevice::hasRequiredExtensions(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if all required physicalDevice extensions are available
	auto availableDeviceExtensions =
	    physicalDevice.enumerateDeviceExtensionProperties();

	auto isSupported = [&](const char *extensionName) {
		return std::ranges::any_of(
		    availableDeviceExtensions, [&](auto const &prop) {
			    return std::string_view(prop.extensionName) == extensionName;
		    });
	};

	bool result = std::ranges::all_of(deviceExtensions, isSupported);
	if (!result) {
		std::cerr << "\t" << " - Missing required extensions" << std::endl;
	}

	return result;
}

bool VulkanDevice::hasRequiredFeatures(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	// Check if the physicalDevice supports the required features (dynamic
	// rendering and extended dynamic state)
	auto features = physicalDevice.template getFeatures2<
	    vk::PhysicalDeviceFeatures2,
	    vk::PhysicalDeviceVulkan13Features,
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	bool result =
	    features.template get<vk::PhysicalDeviceFeatures2>()
	        .features.samplerAnisotropy &&
	    features.template get<vk::PhysicalDeviceVulkan13Features>()
	        .dynamicRendering &&
	    features.template get<vk::PhysicalDeviceVulkan13Features>()
	        .synchronization2 &&
	    features
	        .template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
	        .extendedDynamicState;
	if (!result) {
		std::cerr << "\t" << " - Does not support required features"
		          << std::endl;
	}

	return result;
}

bool VulkanDevice::hasSwapchainSupport(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	SwapChainSupportDetails swapChainSupport =
	    querySwapChainSupport(physicalDevice);
	bool result = !swapChainSupport.formats.empty() &&
	              !swapChainSupport.presentModes.empty();
	if (!result) {
		std::cerr << "\t" << " - Does not support required swapchain support"
		          << std::endl;
	}

	return result;
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	QueueFamilyIndices indices;

	// Get all queue families
	std::vector<vk::QueueFamilyProperties> queueFamilies =
	    physicalDevice.getQueueFamilyProperties();

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

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(
    vk::raii::PhysicalDevice const &physicalDevice) const
{
	SwapChainSupportDetails details;
	details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface_);
	details.formats      = physicalDevice.getSurfaceFormatsKHR(*surface_);
	details.presentModes = physicalDevice.getSurfacePresentModesKHR(*surface_);

	return details;
}

}        // namespace mvr