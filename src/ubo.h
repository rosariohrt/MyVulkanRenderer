#pragma once

#include <glm/glm.hpp>

namespace mvr
{

struct PushConstantObject {
	glm::mat4 model;
};

struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};

}        // namespace mvr
