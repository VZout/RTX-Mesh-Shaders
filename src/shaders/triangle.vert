#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifndef OPTIMIZED_FULLSCREEN
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;
#endif

layout(location = 0) out float g_time;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
    float time;
} ubo;

vec2 positions[4] = vec2[](
    vec2(-1, 1),
    vec2(-1, -1),
    vec2(1, 1),
    vec2(1, -1)
);

vec3 colors[4] = vec3[](
    vec3(1.0, 0.2, 0.2),
    vec3(1.0, 0.2, 0.2),
    vec3(1.0, 0.2, 0.2),
    vec3(1.0, 0.2, 0.2)
);

#ifdef OPTIMIZED_FULLSCREEN
void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    frag_color = colors[gl_VertexIndex];
}
#else
void main()
{
    if (false)
    {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    }
    else
    {
        gl_Position = vec4(pos, 0.0, 1.0);

    }

    g_time = ubo.time;
}
#endif
