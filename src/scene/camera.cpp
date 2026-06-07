#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace mvr
{

namespace
{
// Derive yaw/pitch (degrees) from a normalized direction in the Z-up
// convention used by updateCameraVectors().
float yawFromDirection(glm::vec3 dir)
{
	return glm::degrees(glm::atan(dir.y, dir.x));
}
float pitchFromDirection(glm::vec3 dir)
{
	return glm::degrees(glm::asin(dir.z));
}
}        // namespace

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up) :
    position{position},
    worldUp{up},
    yaw{yawFromDirection(glm::normalize(target - position))},
    pitch{pitchFromDirection(glm::normalize(target - position))},
    movementSpeed{5.0f},
    mouseSensitivity{0.15f},
    scrollSensitivity{1.5f},
    scrollMoveSensitivity{0.1f},
    zoom{45.0f}
{
	updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime)
{
	float velocity = movementSpeed * deltaTime;
	switch (direction) {
		case CameraMovement::Forward:
			position += front * velocity;
			break;
		case CameraMovement::Backward:
			position -= front * velocity;
			break;
		case CameraMovement::Left:
			position -= right * velocity;
			break;
		case CameraMovement::Right:
			position += right * velocity;
			break;
	}
}

void Camera::processMouseMovement(float xOffset,
                                  float yOffset,
                                  bool  constrainPitch)
{
	yaw += xOffset * mouseSensitivity;
	pitch += yOffset * mouseSensitivity;

	if (constrainPitch) {
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	updateCameraVectors();
}

void Camera::processMouseScrollZoom(float yOffset)
{
	zoom -= yOffset * scrollSensitivity;
	if (zoom < 1.0f)
		zoom = 1.0f;
	if (zoom > 90.0f)
		zoom = 90.0f;
}

void Camera::updateCameraVectors()
{
	front = glm::normalize(glm::vec3{
	    cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
	    sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
	    sin(glm::radians(pitch)),
	});
	right = glm::normalize(glm::cross(front, worldUp));
	up    = glm::normalize(glm::cross(right, front));
}

void Camera::processMouseScrollVertical(float yOffset)
{
	position += worldUp * yOffset * scrollMoveSensitivity;
}

}        // namespace mvr
