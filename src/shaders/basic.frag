#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in float g_time;
layout(location = 1) in vec2 g_uv;
layout(location = 2) in vec3 g_normal;
layout(location = 3) in vec3 g_frag_pos;

layout(location = 0) out vec4 out_color;

const vec2 res = vec2(1280, 1280);
const float zoom = 30;

float Hash21(vec2 p)
{
    p = fract(p * vec2(234.24, 435.346));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

void main()
{
    // constants
    const float specular_strength = 0.5;
    const float ambient = 0.1;
    const vec3 light_color = vec3(1);
    const vec3 light_pos = vec3(0, 0, -10);
    const vec3 view_pos = vec3(2, 2, 2);

    // diffuse
    vec3 norm = normalize(g_normal);
    vec3 light_dir = normalize(light_pos - g_frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * light_color;

    // specular
    vec3 view_dir = normalize(view_pos - g_frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = specular_strength * spec * light_color;

    vec3 color = ambient + diffuse + specular;
    out_color = vec4(color, 1);
}
