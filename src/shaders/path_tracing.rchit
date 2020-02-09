#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#pragma shader_stage(closest)
#define SCATTER

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

#include "random.glsl"
#include "structs.glsl"

layout(location = 0) rayPayloadInNV NewPayload payload;
layout(location = 1) rayPayloadNV bool shadow_payload;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV scene;

layout(set = 3, binding = 3) uniform UniformBufferLightObject {
    Light lights[25];
} lights;

layout(set = 4, binding = 4) buffer VertexBufferObj {
	Vertex vertices[];
} vb;

layout(set = 5, binding = 5) buffer IndexBufferObj {
	uint indices[];
} ib;

layout(set = 6, binding = 6) buffer UniformBufferOffsetObject
{ 
    RaytracingOffset offsets[];
} offsets;

layout(set = 7, binding = 7) buffer UniformBufferMaterialObject
{ 
    RaytracingMaterial materials[];
} materials;

layout(set = 8, binding = 8) uniform sampler2D ts_textures[100];

layout(set = 10, binding = 10) uniform sampler2D t_brdf_lut;

#include "pbr_disney.glsl"
#include "rt_sample.glsl"

ReadableVertex VertexToReadable(Vertex vertex)
{
	ReadableVertex retval;
	retval.pos = vec3(vertex.x, vertex.y, vertex.z);
	retval.uv = vec2(vertex.u, vertex.v);
	retval.normal = vec3(vertex.nx, vertex.ny, vertex.nz);
	retval.tangent = vec3(vertex.tx, vertex.ty, vertex.tz);
	retval.bitangent = vec3(vertex.bx, vertex.by, vertex.bz);

	return retval;
}

vec3 HitAttribute(vec3 a, vec3 b, vec3 c, vec3 bary)
{
	return a + bary.x * (b - a) + bary.y * (c - a);
}

vec3 HitWorldPosition()
{
	return gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
}

vec3 DirectLighting(Surface surface, vec3 world_pos, vec3 V, vec3 N)
{
	// Shadow
	uint light_count = lights.lights[0].m_type >> 2;
	float shadow_mult = 0;

	if (light_count <= 0)
	{
		return vec3(0);
	}

	uint light_to_sample = min(int(nextRand(payload.seed) * light_count), int(light_count - 1));
	Light light = lights.lights[light_to_sample];
	const vec3 light_dir = light.m_dir;
	const float inner_angle = light.m_inner_angle;
	const float outer_angle = light.m_outer_angle;

	const uint type = light.m_type & 3;
	vec3 pos_to_light = light.m_pos - world_pos;

	// Shadow Ray
	vec3 L = mix(pos_to_light, -light.m_dir, float(light.m_type == DIRECTIONAL_LIGHT));
	float light_dist = length(L);
	L /= light_dist;
	float max_light_dist = mix(light_dist, 100000, float(light.m_type == DIRECTIONAL_LIGHT));

	vec3 shadow_dir = normalize(GetUniformHemisphereSample(payload.seed, L, light.m_physical_size));
	shadow_mult = float(light_count) * float(!TraceShadowRay(world_pos + (L * EPSILON), shadow_dir, max_light_dist, payload.seed));

	vec3 emission = light.m_color;

	float sqr_falloff = light.m_radius * light.m_radius;
	float sqr_inv_radius = sqr_falloff > 0.0f ? (1 / sqr_falloff) : 0;
	float attenuation = GetDistanceAttenuation(pos_to_light, sqr_inv_radius);
	if (type == DIRECTIONAL_LIGHT)
	{
		attenuation = 1;
		L = -light_dir;
	}
	else if (type == SPOT_LIGHT)
	{
		attenuation *= GetAngleAttenuation(-light_dir, L, inner_angle, outer_angle);
	}

	const vec3 H = normalize(V + L);
	const float NdotH = max(0.0, dot(N, H));
	const float NdotL = max(0.0, dot(L, N));
	const float HdotL = max(0.0, dot(H, L));
	const float LdotH = max(0.0, dot(H, L));
	const float NdotV = max(0.0, dot(N, V));

	const float bsdf_pdf = DisneyPdf(surface, NdotH, NdotL, HdotL);

	const vec3 f = DisneyEval(surface, NdotL, NdotV, NdotH, LdotH, V, L, H);

	return (f * emission) * attenuation * shadow_mult;
}

void main()
{
	float t = gl_HitTNV;

	RaytracingOffset offset = offsets.offsets[gl_InstanceCustomIndexNV];
	const uint vertex_offset = offset.vertex_offset / 56;
	const uint index_offset = offset.idx_offset / 4;

	const uint triangle_stride = 3;
	uint base_idx = gl_PrimitiveID * triangle_stride;
	base_idx += index_offset;

	vec3 world_pos = HitWorldPosition();
	vec3 view_pos = gl_WorldRayOriginNV;
	vec3 V = -gl_WorldRayDirectionNV;

	uvec3 indices = uvec3(ib.indices[base_idx+0], ib.indices[base_idx+1], ib.indices[base_idx+2]);
	indices += uvec3(vertex_offset, vertex_offset, vertex_offset); // offset the start

	const ReadableVertex v0 = VertexToReadable(vb.vertices[indices.x]);
	const ReadableVertex v1 = VertexToReadable(vb.vertices[indices.y]);
	const ReadableVertex v2 = VertexToReadable(vb.vertices[indices.z]);

	// Raytracing Material
	RaytracingMaterial material = materials.materials[gl_InstanceCustomIndexNV];

	vec3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attribs));

	const vec3 tangent = normalize(HitAttribute(v0.tangent, v1.tangent, v2.tangent, attribs));
	const vec3 bitangent = normalize(HitAttribute(v0.bitangent, v1.bitangent, v2.bitangent, attribs));
	vec2 uv = HitAttribute(vec3(v0.uv, 0), vec3(v1.uv, 0), vec3(v2.uv, 0), attribs).xy;
	uv.x *= material.u_scale;
	uv.y *= material.v_scale;
	uv.y = 1.0f - uv.y;

	uint lod_level = 0;

	vec3 albedo = material.color.x > -1 ? material.color.rgb : textureLod(ts_textures[material.albedo_texture], uv, lod_level).rgb;
	vec3 emissive = textureLod(ts_textures[material.emissive_texture], uv,lod_level).rgb;
	vec3 normal_t = normalize(textureLod(ts_textures[material.normal_texture], uv, lod_level).rgb * 2.0f - 1.0f);
	vec4 compressed_mra = textureLod(ts_textures[material.roughness_texture], uv, lod_level).rgba;

	vec3 geometric_normal = vec3(0);
	vec3 anisotropic_t = vec3(0);
	vec3 N = CalcPeturbedNormal(normal, normal_t, tangent, bitangent, V, geometric_normal, vec3(material.anisotropy_x, material.anisotropy_y, 0), anisotropic_t);

	if (material.two_sided != 0 && dot(gl_WorldRayDirectionNV, N) > 0 && dot(gl_WorldRayDirectionNV, geometric_normal) > 0) // Flip normal to direction
	{
		geometric_normal = -geometric_normal;
		N = -N;
	}

	float thickness = textureLod(ts_textures[material.thickness_texture], uv, lod_level).r;;
    float metallic = material.metallic > -1 ? material.metallic : compressed_mra.b;
	float roughness = clamp(material.roughness > -1 ? material.roughness : compressed_mra.g, MIN_PERCEPTUAL_ROUGHNESS, 1.f);
	float anisotropy = material.anisotropy;
	float clear_coat = material.clear_coat;
	float cc_roughness = clamp(material.clear_coat_roughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);;
	float reflectivity = material.reflectivity;
	vec3 subsurface_color = vec3(24 / 255.f, 69 / 255.f, 0);
	float subsurface_power = 0;

	vec3 diffuse_color = ComputeDiffuseColor(albedo, metallic);
    float reflectance = ComputeDielectricF0(reflectivity); // TODO: proper material reflectance
    vec3 F0 = ComputeF0(albedo, metallic, reflectance);
	float NdotV = max(dot(N, V), MIN_N_DOT_V);

	// Surface
	Surface surface;
	surface.albedo = albedo;
	surface.metallic = metallic;
	surface.roughness = roughness;
	surface.specular = ProbabilityToSampleDiffuse(albedo, vec3(metallic));
	surface.clearcoat = clear_coat;
	surface.clearcoat_gloss = abs(cc_roughness - 1);
	if (surface.clearcoat == 0)
		surface.clearcoat_gloss = 0;
	surface.thickness = 0;
	surface.anisotropic = anisotropy;
	surface.sheen = 0;
	surface.aniso_t = anisotropic_t;
	surface.aniso_b = normalize(cross(geometric_normal, anisotropic_t));
	CalculateCSW(surface);

	vec3 color = vec3(0);
	vec3 throughput = payload.throughput;
	

	// Direct Lighting
	vec3 lighting = DirectLighting(surface, world_pos, V, N);
	color += clamp((emissive * 2) * throughput, 0, 10);
	color += clamp(lighting * throughput, 0, 10);
	//color += lighting * throughput;

	const vec3 bsdf_dir = DisneySample(surface, payload.seed, V, N);

	const vec3 L = bsdf_dir;
	const vec3 H = normalize(V + L);

    const float NdotL = clamp(dot(N, L), 0.0, 1.0);
    const float NdotH = clamp(dot(N, H), 0.0, 1.0);
    const float HdotL = clamp(dot(H, L), 0.0, 1.0);
	const float LdotH = clamp(dot(H, L), 0.0, 1.0);

	//const float NdotL = abs(dot(N, L));
    //const float NdotH = abs(dot(N, H));
    //const float HdotL = abs(dot(H, L));
	//const float LdotH = abs(dot(H, L));

	const float pdf = DisneyPdf(surface, NdotH, NdotL, HdotL);
	if (pdf > 0.0)
	{
		throughput *= DisneyEval(surface, NdotL, NdotV, NdotH, LdotH, V, L, H) / pdf;
	}
	else
	{
		t = -1.f;
	}


	payload.color_t = vec4(color, t);
	payload.throughput = throughput;
	payload.direction = bsdf_dir;
	payload.normal = N;
}
