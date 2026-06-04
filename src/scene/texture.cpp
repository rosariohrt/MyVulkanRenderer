#define STB_IMAGE_IMPLEMENTATION

#include "texture.h"

// std
#include <stdexcept>
#include <tuple>

// libs
#include <stb_image.h>

namespace mvr
{

Texture::Texture(VulkanDevice &device, const std::string &filePath) :
    device{device}
{
	createTextureImage(filePath);
	createTextureImageView();
	createTextureSampler();
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory>
    Texture::createImage(uint32_t                width,
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

	vk::raii::Image image = vk::raii::Image(device.device(), imageInfo);

	vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo       = {
	    .allocationSize = memRequirements.size,
	    .memoryTypeIndex =
	        device.findMemoryType(memRequirements.memoryTypeBits, properties),
	};

	vk::raii::DeviceMemory imageMemory =
	    vk::raii::DeviceMemory(device.device(), allocInfo);
	image.bindMemory(*imageMemory, 0);

	return {std::move(image), std::move(imageMemory)};
}

void Texture::createTextureImage(const std::string &filePath)
{
	int      texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load(
	    filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	auto [stagingBuffer, stagingBufferMemory] =
	    device.createBuffer(imageSize,
	                        vk::BufferUsageFlagBits::eTransferSrc,
	                        vk::MemoryPropertyFlagBits::eHostVisible |
	                            vk::MemoryPropertyFlagBits::eHostCoherent);

	void *dataStaging = stagingBufferMemory.mapMemory(0, imageSize);
	memcpy(dataStaging, pixels, imageSize);
	stagingBufferMemory.unmapMemory();

	stbi_image_free(pixels);

	std::tie(textureImage, textureImageMemory) = createImage(
	    texWidth,
	    texHeight,
	    vk::Format::eR8G8B8A8Srgb,
	    vk::ImageTiling::eOptimal,
	    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
	    vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::raii::CommandBuffer commandBuffer = device.beginSingleTimeCommands();
	transitionImageLayout(commandBuffer,
	                      textureImage,
	                      vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eTransferDstOptimal);
	device.copyBufferToImage(commandBuffer,
	                         stagingBuffer,
	                         textureImage,
	                         static_cast<uint32_t>(texWidth),
	                         static_cast<uint32_t>(texHeight));
	transitionImageLayout(commandBuffer,
	                      textureImage,
	                      vk::ImageLayout::eTransferDstOptimal,
	                      vk::ImageLayout::eShaderReadOnlyOptimal);
	device.endSingleTimeCommands(std::move(commandBuffer));
}

void Texture::createTextureImageView()
{
	vk::ImageViewCreateInfo viewInfo = {
	    .image    = textureImage,
	    .viewType = vk::ImageViewType::e2D,
	    .format   = vk::Format::eR8G8B8A8Srgb,
	    .subresourceRange =
	        {
	            .aspectMask     = vk::ImageAspectFlagBits::eColor,
	            .baseMipLevel   = 0,
	            .levelCount     = 1,
	            .baseArrayLayer = 0,
	            .layerCount     = 1,
	        },
	};
	textureImageView = vk::raii::ImageView(device.device(), viewInfo);
}

void Texture::createTextureSampler()
{
	vk::PhysicalDeviceProperties properties =
	    device.physicalDevice().getProperties();

	vk::SamplerCreateInfo samplerInfo = {
	    .magFilter               = vk::Filter::eLinear,
	    .minFilter               = vk::Filter::eLinear,
	    .mipmapMode              = vk::SamplerMipmapMode::eLinear,
	    .addressModeU            = vk::SamplerAddressMode::eRepeat,
	    .addressModeV            = vk::SamplerAddressMode::eRepeat,
	    .addressModeW            = vk::SamplerAddressMode::eRepeat,
	    .mipLodBias              = 0.0f,
	    .anisotropyEnable        = vk::True,
	    .maxAnisotropy           = properties.limits.maxSamplerAnisotropy,
	    .compareEnable           = vk::False,
	    .compareOp               = vk::CompareOp::eAlways,
	};
	textureSampler = vk::raii::Sampler(device.device(), samplerInfo);
}

void Texture::transitionImageLayout(vk::raii::CommandBuffer &commandBuffer,
                                    const vk::raii::Image   &image,
                                    vk::ImageLayout          oldLayout,
                                    vk::ImageLayout          newLayout)
{
	vk::ImageMemoryBarrier barrier = {
	    .oldLayout           = oldLayout,
	    .newLayout           = newLayout,
	    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
	    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
	    .image               = image,
	    .subresourceRange =
	        {
	            .aspectMask = vk::ImageAspectFlagBits::eColor,
	            .levelCount = 1,
	            .layerCount = 1,
	        },
	};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined &&
	    newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
	           newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage      = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	commandBuffer.pipelineBarrier(
	    sourceStage, destinationStage, {}, {}, nullptr, barrier);
}

}        // namespace mvr