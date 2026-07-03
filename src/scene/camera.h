#pragma once

// libs
#include <glm/glm.hpp>

namespace mvr
{

enum class CameraMovement
{
	Forward,
	Backward,
	Left,
	Right
};

class Camera
{
  public:
	// Positioned at `position`. World is Y-up.
	// Initial orientation faces -Z (yaw = -90, pitch = 0).
	Camera(glm::vec3 position = {0.0f, 0.0f, 0.0f});

	glm::mat4 getViewMatrix() const;

	void processKeyboard(CameraMovement direction, float deltaTime);
	void processMouseMovement(float xOffset,
	                          float yOffset,
	                          bool  constrainPitch = true);
	void processMouseScrollZoom(float yOffset);
	void processMouseScrollVertical(float yOffset);

	// Getters
	glm::vec3 getPosition() const
	{
		return position;
	}
	glm::vec3 getFront() const
	{
		return front;
	}
	float getZoom() const
	{
		return zoom;
	}

  private:
	static constexpr float YAW   = -90.0f;
	static constexpr float PITCH = 0.0f;

	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;
	float     yaw;
	float     pitch;
	float     movementSpeed;
	float     mouseSensitivity;
	float     scrollSensitivity;
	float     scrollMoveSensitivity;
	float     zoom;

	void updateCameraVectors();
};

}        // namespace mvr
