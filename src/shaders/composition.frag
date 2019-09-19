#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D ts_gbuffers[3];

layout(location = 0) in vec2 g_uv;

layout(location = 0) out vec4 out_color;

const vec2 res = vec2(1280, 1280);
const float zoom = 5;


void main()
{
    // GBuffers
    const vec3 albedo = texture(ts_gbuffers[0], g_uv).rgb;
    const vec3 normal = texture(ts_gbuffers[1], g_uv).rgb;
    const vec3 world_pos = texture(ts_gbuffers[2], g_uv).xyz;

    // Constants
    const float specular_strength = 0.5;
    const float ambient = 0.4;
    const vec3 light_color = vec3(1);
    const vec3 view_pos = vec3(0, 0, -2.5);
    const vec3 light_pos = view_pos;

    // Diffuse
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(light_pos - world_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * light_color;

    // Specular
    vec3 view_dir = normalize(view_pos - world_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = specular_strength * spec * light_color;

    vec3 color = (ambient + diffuse + specular) * albedo;

    out_color = vec4(color, 1.0);
}
