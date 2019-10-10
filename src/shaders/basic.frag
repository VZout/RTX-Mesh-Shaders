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
layout(location = 3) out vec4 out_material;
layout(location = 4) out vec4 out_anisotropy;

layout(set = 3, binding = 3) uniform UniformBufferObject {
    vec3 color;
    float reflectivity;
    float roughness;
    float metallic;
    float normal_strength;
    float anisotropy;
	vec3 anisotropy_dir;
	float unused;
} material;

void main()
{
    vec3 compressed_mra = texture(ts_textures[2], g_uv).rgb;

    vec3 normal = normalize(g_normal);
    mat3 TBN = mat3( normalize(g_tangent), normalize(g_bitangent), normal );
    vec3 normal_t = normalize(texture(ts_textures[1], g_uv).xyz * 2.0f - 1.0f);

    vec4 albedo = material.color.x > -1 ? vec4(material.color, 1) : texture(ts_textures[0], g_uv);
    vec3 obj_normal = normalize(TBN * (normal_t * material.normal_strength));
    float roughness = material.roughness > -1 ? material.roughness : compressed_mra.g;
    float metallic = material.metallic > -1 ? material.metallic : compressed_mra.b;
    float ao = compressed_mra.r;

	if (albedo.a < 0.5)
		discard;

    // disable normal mapping
    obj_normal = material.normal_strength > -1 ? obj_normal : normal;

    out_color = vec4(albedo.rgb, roughness);
    out_normal = vec4(obj_normal, metallic);
    out_pos = vec4(g_frag_pos, ao);

	vec3 anisotropic_t = normalize(normalize(g_tangent) * material.anisotropy_dir);
	vec3 anisotropic_b = normalize(cross(normal, anisotropic_t));

    out_material = vec4(material.reflectivity, material.anisotropy, anisotropic_t.rg);
	out_anisotropy = vec4(anisotropic_t.b, anisotropic_b);
}
