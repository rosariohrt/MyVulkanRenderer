#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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

	bool shouldClose();
	void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

  private:
	GLFWwindow    *window;
	std::string    windowName;
	const uint32_t width;
	const uint32_t height;

	void initWindow();
};

}        // namespace mvr