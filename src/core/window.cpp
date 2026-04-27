#include "window.h"

// std
#include <stdexcept>

namespace mvr
{

Window::Window(int w, int h, std::string name) :
    width{w}, height{h}, windowName{name}
{
	initWindow();
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::resetWindowResizedFlag()
{
	framebufferResized = false;
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}

void Window::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Window::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
	auto pWindow                = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
	pWindow->framebufferResized = true;
	pWindow->width              = width;
	pWindow->height             = height;
}

}        // namespace mvr
