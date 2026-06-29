#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

struct DirectionalLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4             view;
    mat4             proj;
    vec4             viewPos;
    DirectionalLight light;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main()
{
    fragPos = vec3(push.model * vec4(pos, 1.0));
    fragNormal = mat3(transpose(inverse(push.model))) * normal;
    fragTexCoord = texCoord;

    gl_Position = ubo.proj * ubo.view * push.model * vec4(pos, 1.0);
}