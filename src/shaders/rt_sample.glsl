#ifndef RT_UTIL_GLSL
#define RT_UTIL_GLSL

#include "structs.glsl"

vec3 TraceColorRay(vec3 origin, vec3 direction, int light, float shadow_mult, uint seed)
{
	uint flags = gl_RayFlagsCullBackFacingTrianglesNV;
	float tmin = 0.001;
	float tmax = 10000.0;

	payload.seed = seed;
	payload.color_t.w = -1;
	payload.light = light;
	payload.shadow_mult = shadow_mult;
	traceNV(scene, flags, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);

	return payload.color_t.rgb;
}

bool TraceShadowRay(vec3 origin, vec3 direction, float max_dist, uint seed)
{
	uint flags = gl_RayFlagsCullBackFacingTrianglesNV;
	float tmin = 0.001;
	float tmax = max_dist;

	payload.seed = seed;
	traceNV(scene, flags, 0xff, 1, 0, 1, origin, tmin, direction, tmax, 1);

	return shadow_payload;
}

#ifndef RAYGEN
#ifndef MISS
vec3 CalcPeturbedNormal(vec3 normal, vec3 normal_map, vec3 tangent, vec3 bitangent, vec3 V, out vec3 world_normal, vec3 aniso_dir, out vec3 aniso_t)
{
	mat4x3 object_to_world = gl_ObjectToWorldNV;
	vec3 N = normalize(object_to_world * vec4(normal, 0)).xyz;
	vec3 T = normalize(object_to_world * vec4(tangent, 0)).xyz;
#define CALC_BITANGENT
#ifndef CALC_BITANGENT
	const vec3 B = normalize(object_to_world * vec4(bitangent, 0)).xyz;
#else
	vec3 B = cross(N, T);
#endif
	
	const mat3 TBN = mat3(T, B, N);
	vec3 fN = normalize(TBN * normal_map);
	aniso_t = normalize(TBN * normalize(aniso_dir));

	world_normal = N;

	return fN;
}
#endif /* miss */
#endif /* raygen */
#endif /* RT_UTIL_GLSL */
