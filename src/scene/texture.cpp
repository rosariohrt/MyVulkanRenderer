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
}

std::pair<vk::raii::Image,
          vk::raii::DeviceMemory>
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
	    .allocationSize  = memRequirements.size,
	    .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties),
	};

	vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device.device(), allocInfo);
	image.bindMemory(*imageMemory, 0);

	return {std::move(image), std::move(imageMemory)};
}

void Texture::createTextureImage(const std::string &filePath)
{
	int            texWidth, texHeight, texChannels;
	stbi_uc       *pixels    = stbi_load(filePath.c_str(),
	                                     &texWidth,
	                                     &texHeight,
	                                     &texChannels,
	                                     STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	auto [stagingBuffer, stagingBufferMemory] = device.createBuffer(
	    imageSize,
	    vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	void *dataStaging = stagingBufferMemory.mapMemory(0, imageSize);
	memcpy(dataStaging, pixels, imageSize);
	stagingBufferMemory.unmapMemory();

	stbi_image_free(pixels);

	std::tie(TextureImage, TextureImageMemory) = createImage(
	    texWidth,
	    texHeight,
	    vk::Format::eR8G8B8A8Srgb,
	    vk::ImageTiling::eOptimal,
	    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
	    vk::MemoryPropertyFlagBits::eDeviceLocal);
}

}        // namespace mvr