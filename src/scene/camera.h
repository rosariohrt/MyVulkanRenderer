#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
	Camera(glm::vec3 position = {0.0f, 0.0f, 0.0f},
	       glm::vec3 up       = {0.0f, 0.0f, 1.0f},
	       float     yaw      = -90.0f,
	       float     pitch    = 0.0f);

	glm::mat4 getViewMatrix() const;

	void processKeyboard(CameraMovement direction, float deltaTime);
	void processMouseMovement(float xOffset,
	                          float yOffset,
	                          bool  constrainPitch = true);
	void processMouseScrollZoom(float yOffset);

	glm::vec3 getPosition() const { return position; }
	glm::vec3 getFront() const { return front; }
	float     getZoom() const { return zoom; }

  private:
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
	float     zoom;

	void updateCameraVectors();
};

}        // namespace mvr
