#ifndef RT_UTIL_GLSL
#define RT_UTIL_GLSL

#include "structs.glsl"

vec3 TraceColorRay(vec3 origin, vec3 direction, uint seed, uint depth)
{
	uint flags = 0;
	float tmin = 0.001;
	float tmax = 10000.0;

	payload.seed = seed;
	payload.depth = depth;

	if (depth >= MAX_RECURSION)
	{
		return vec3(0);
	}

	traceNV(scene, flags, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);

	return payload.color;
}

#ifndef RAYGEN
bool TraceShadowRay(vec3 origin, vec3 direction, float max_dist, uint seed, uint depth)
{
	uint flags = 0;
	float tmin = 0.001;
	float tmax = max_dist;

	if (depth >= MAX_RECURSION)
	{
		return false;
	}

	traceNV(scene, flags, 0xff, 1, 0, 1, origin, tmin, direction, tmax, 1);

	return shadow_payload;
}

#ifndef MISS
vec3 CalcPeturbedNormal(vec3 normal, vec3 normal_map, vec3 tangent, vec3 bitangent, vec3 V, out vec3 world_normal)
{
	mat4x3 object_to_world = gl_ObjectToWorldNV;
	vec3 N = normalize(object_to_world * vec4(normal, 0)).xyz;
	vec3 T = normalize(object_to_world * vec4(tangent, 0)).xyz;
#ifndef CALC_BITANGENT
	const vec3 B = normalize(object_to_world * vec4(bitangent, 0)).xyz;
#else
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
#endif
	const mat3 TBN = mat3(T, B, N);

	vec3 fN = normalize(TBN * normal_map);

	world_normal = N;

	return fN;
}
#endif /* miss */
#endif /* raygen */
#endif /* RT_UTIL_GLSL */
