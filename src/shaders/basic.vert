#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(location = 0) out float g_time;
layout(location = 1) out vec2 g_uv;
layout(location = 2) out vec3 g_normal;
layout(location = 3) out vec3 g_frag_pos;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
    float time;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    g_frag_pos = vec3(ubo.model * vec4(pos, 1.0));
    g_time = ubo.time;
    g_uv = uv;
    g_uv.y *= -1;
    g_normal = mat3(ubo.model) * normal;
}