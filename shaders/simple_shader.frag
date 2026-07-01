#version 450

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
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;
    vec3 normal   = normalize(fragNormal);
    vec3 lightDir = normalize(-ubo.light.direction.xyz);
    vec3 viewDir  = normalize(ubo.viewPos.xyz - fragPos);

    // ambient
    vec3 ambient = ubo.light.ambient.rgb * texColor;

    // diffuse
    float diff    = max(dot(normal, lightDir), 0.0);
    vec3  diffuse = ubo.light.diffuse.rgb * diff * texColor;

    // specular (Blinn-Phong)
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3  specular = ubo.light.specular.rgb * spec;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
