#include "input_manager.h"

namespace mvr
{

namespace
{
// GLFW scroll callbacks are C function pointers and the window user pointer is
// already owned by Window, so route the callback through a single instance
// pointer. Assumes one InputManager exists at a time.
InputManager *s_instance = nullptr;
}        // namespace

InputManager::InputManager(Window &window) : window{window.getGLFWwindow()}
{
	s_instance = this;
	glfwSetScrollCallback(this->window, scrollCallback);
}

void InputManager::update()
{
	double xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);
	glm::vec2 pos{static_cast<float>(xPos), static_cast<float>(yPos)};

	if (firstMouse) {
		lastMousePos = pos;
		firstMouse   = false;
	}

	mouseDelta   = pos - lastMousePos;
	lastMousePos = pos;
}

bool InputManager::isKeyPressed(int key) const
{
	return glfwGetKey(window, key) == GLFW_PRESS;
}

bool InputManager::isMouseButtonPressed(int button) const
{
	return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

float InputManager::consumeScrollDelta()
{
	float delta = scrollDelta;
	scrollDelta = 0.0f;
	return delta;
}

void InputManager::scrollCallback(GLFWwindow * /*window*/,
                                  double /*xOffset*/,
                                  double yOffset)
{
	if (s_instance) {
		s_instance->scrollDelta += static_cast<float>(yOffset);
	}
}

}        // namespace mvr
