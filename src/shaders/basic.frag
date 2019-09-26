#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 2, binding = 2) uniform sampler2D ts_textures[3];

layout(location = 0) in vec2 g_uv;
layout(location = 1) in vec3 g_normal;
layout(location = 2) in vec3 g_frag_pos;
layout(location = 3) in vec3 g_tangent;
layout(location = 4) in vec3 g_bitangent;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_pos;

void main()
{
    vec3 normal = normalize(g_normal);
    normal.y = -normal.y;
    mat3 TBN = mat3( normalize(g_tangent), normalize(g_bitangent), normal );
    vec3 normal_t = normalize(texture(ts_textures[1], g_uv).xyz * 2.0f - 1.0f);
    vec3 obj_normal = TBN * normal_t;
    vec3 albedo = texture(ts_textures[0], g_uv).xyz;

    float ao = 0;
    float metallic = 0;
    float roughness = 0;
    if (true)
    {
        vec3 compressed_mra = texture(ts_textures[2], g_uv).rgb;
        roughness = compressed_mra.g;
        metallic = compressed_mra.b;
        ao = compressed_mra.r;
    }

    out_color = vec4(albedo, roughness);
    out_normal = vec4(obj_normal, metallic);
    out_pos = vec4(g_frag_pos, ao);
}
