#pragma once

#include "window.h"

#include <glm/glm.hpp>

namespace mvr
{

class InputManager
{
  public:
	explicit InputManager(Window &window);

	InputManager(const InputManager &)            = delete;
	InputManager &operator=(const InputManager &) = delete;

	void update();

	bool isKeyPressed(int key) const;
	bool isMouseButtonPressed(int button) const;

	// Cursor movement since the previous update(), in screen pixels
	// (x: right positive, y: down positive).
	glm::vec2 getMouseDelta() const
	{
		return mouseDelta;
	}

	float consumeScrollDelta();

  private:
	GLFWwindow *window;

	glm::vec2 lastMousePos{0.0f, 0.0f};
	glm::vec2 mouseDelta{0.0f, 0.0f};
	bool      firstMouse = true;

	float scrollDelta = 0.0f;

	static void
	    scrollCallback(GLFWwindow *window, double xOffset, double yOffset);
};

}        // namespace mvr
