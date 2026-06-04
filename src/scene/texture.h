#pragma once

#include "core/vulkan_device.h"

// std
#include <string>

namespace mvr
{

class Texture
{
  public:
	Texture(VulkanDevice &device, const std::string &filePath);
	~Texture();

	Texture(const Texture &)            = delete;
	Texture &operator=(const Texture &) = delete;

  private:
	VulkanDevice          &device;
	vk::raii::Image        textureImage       = nullptr;
	vk::raii::DeviceMemory textureImageMemory = nullptr;
	vk::raii::ImageView    textureImageView   = nullptr;

	std::pair<vk::raii::Image, vk::raii::DeviceMemory>
	     createImage(uint32_t                width,
	                 uint32_t                height,
	                 vk::Format              format,
	                 vk::ImageTiling         tiling,
	                 vk::ImageUsageFlags     usage,
	                 vk::MemoryPropertyFlags properties);
	void createTextureImage(const std::string &filePath);
	void createTextureImageView();

	void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer,
	                           const vk::raii::Image   &image,
	                           vk::ImageLayout          oldLayout,
	                           vk::ImageLayout          newLayout);
};

}        // namespace mvr