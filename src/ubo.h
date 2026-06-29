#pragma once

#include <glm/glm.hpp>

namespace mvr
{

struct PushConstantObject {
	glm::mat4 model;
};

struct DirectionalLight {
	// w is unused, but we need to keep the struct size
	// a multiple of 16 bytes for alignment
	glm::vec4 direction;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct UniformBufferObject {
	glm::mat4        view;
	glm::mat4        proj;
	glm::vec4        viewPos;
	DirectionalLight light;
};

}        // namespace mvr
