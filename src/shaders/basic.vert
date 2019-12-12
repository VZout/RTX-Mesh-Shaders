#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(location = 0) out vec2 g_uv;
layout(location = 1) out vec3 g_normal;
layout(location = 2) out vec3 g_frag_pos;
layout(location = 3) out vec3 g_tangent;
layout(location = 4) out vec3 g_bitangent;

// Uniforms
layout(set = 1, binding = 1) uniform UniformBufferObject {
    mat4 model[100];
} ubo;

layout(set = 0, binding = 0) uniform UniformBufferCameraObject {
    mat4 view;
    mat4 proj;
} camera;

void main()
{
    mat4 model = ubo.model[gl_InstanceIndex];
    g_tangent = normalize(model * vec4(tangent, 0)).xyz;
    g_bitangent = normalize(model * vec4(bitangent, 0)).xyz;
    g_normal = normalize(model * vec4(normal, 0)).xyz;

    g_frag_pos = vec3(model * vec4(pos, 1.0));
	g_frag_pos = pos;
    gl_Position = camera.proj * camera.view * model * vec4(pos, 1.0);
    g_uv = uv;
    g_uv.y *= -1;
}
