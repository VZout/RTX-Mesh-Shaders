#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 g_uv;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
    float time;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec2 positions[4] = vec2[](
    vec2(-1, 1),
    vec2(-1, -1),
    vec2(1, 1),
    vec2(1, -1)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    g_uv = 0.5 * (positions[gl_VertexIndex].xy + vec2(1.0));
}