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

	Texture(const Texture &)            = delete;
	Texture &operator=(const Texture &) = delete;

	vk::DescriptorImageInfo getDescriptorImageInfo() const;

  private:
	VulkanDevice          &device;
	vk::raii::Image        textureImage       = nullptr;
	vk::raii::DeviceMemory textureImageMemory = nullptr;
	vk::raii::ImageView    textureImageView   = nullptr;
	vk::raii::Sampler      textureSampler     = nullptr;

	void createTextureImage(const std::string &filePath);
	void createTextureImageView();
	void createTextureSampler();

	void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer,
	                           const vk::raii::Image   &image,
	                           vk::ImageLayout          oldLayout,
	                           vk::ImageLayout          newLayout);
};

}        // namespace mvr