#version 460
#extension GL_NV_ray_tracing : require
#pragma shader_stage(miss)
#define MISS

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

#include "structs.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureNV scene;

layout(set = 9, binding = 9) uniform samplerCube t_skybox;

layout(location = 0) rayPayloadInNV NewPayload payload;
layout(location = 1) rayPayloadNV bool shadow_payload;

layout(set = 3, binding = 3) uniform UniformBufferLightObject {
    Light lights[25];
} lights;

void main()
{
	vec3 sky = texture(t_skybox, gl_WorldRayDirectionNV).rgb;

	payload.color_t = clamp(vec4(payload.throughput * sky, -1), 0, 10);
	//payload.color_t = vec4(payload.throughput * sky, -1);
	payload.throughput = vec3(0);
}
