#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 frag_color;

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

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    frag_color = colors[gl_VertexIndex];
}
