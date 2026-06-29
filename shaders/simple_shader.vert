#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
    gl_Position = ubo.proj * ubo.view * push.model * vec4(pos, 1.0);
    fragTexCoord = texCoord;
}