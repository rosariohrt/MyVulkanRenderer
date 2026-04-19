#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

// std
#include <string>

namespace mvr
{

class Window
{
  public:
	Window(uint32_t w, uint32_t h, std::string name);
	~Window();

	Window(const Window &)            = delete;
	Window &operator=(const Window &) = delete;

	bool         shouldClose();
	vk::Extent2D getExtent();
	void         getFrameBufferSize(int *width, int *height);

	void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

  private:
	GLFWwindow    *window;
	std::string    windowName;
	const uint32_t width;
	const uint32_t height;

	void initWindow();
};

}        // namespace mvr