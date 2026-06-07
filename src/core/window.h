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
	Window(int w, int h, std::string name);
	~Window();

	Window(const Window &)            = delete;
	Window &operator=(const Window &) = delete;

	void setShouldClose(bool value);
	void resetWindowResizedFlag();
	void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

	bool shouldClose()
	{
		return glfwWindowShouldClose(window);
	}
	vk::Extent2D getExtent()
	{
		return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
	}
	bool wasWindowResized()
	{
		return framebufferResized;
	}

	GLFWwindow *getGLFWwindow() const
	{
		return window;
	}

  private:
	GLFWwindow *window;
	std::string windowName;

	int  width;
	int  height;
	bool framebufferResized = false;

	void initWindow();
	static void
	    framebufferResizeCallback(GLFWwindow *window, int width, int height);
};

}        // namespace mvr