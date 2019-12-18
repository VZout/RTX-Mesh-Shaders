#include "structs.glsl"
#include "pbr+_util.glsl"

vec3 ShadeLight(Light light,
	vec3 world_pos,
	vec3 N,
	vec3 geometric_normal,
	vec3 V,
	float roughness,
	vec3 diffuse_color,
	float metallic,
	float thickness,
	float clear_coat,
	float cc_roughness,
	float subsurface_power,
	vec3 subsurface_color,
	float anisotropy,
	vec3 anisotropic_t,
	vec3 anisotropic_b,
	vec3 F0,
	vec3 energy_compensation)
{
	vec3 lighting = vec3(0);

	const vec3 light_color = light.m_color;
	const vec3 light_pos = light.m_pos;
	const vec3 light_dir = light.m_dir;
	const uint type = light.m_type & 3;
	const float inner_angle = light.m_inner_angle;
	const float outer_angle = light.m_outer_angle;

	const vec3 pos_to_light = light_pos - world_pos;
	vec3 L = normalize(pos_to_light);

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
		attenuation *= GetAngleAttenuation(light_dir, L, inner_angle, outer_angle);
	}

	float NdotL = clamp(dot(N, L), 0.0, 1.0); // TODO: Duplicate ndotl

#ifdef SCATTER
	if (thickness < 1)
	{
		lighting = SS_BRDF(L, V, N, geometric_normal, metallic, roughness, diffuse_color, light_color, F0, energy_compensation, attenuation, 1.0f, thickness, subsurface_color, subsurface_power);
	}
	else
	{
		if (NdotL > 0.0)
		{
			lighting = BRDF(L, V, N, geometric_normal, metallic, roughness, diffuse_color, light_color, F0, energy_compensation, attenuation, 1.0f, anisotropy, anisotropic_t, anisotropic_b, clear_coat, cc_roughness);
		}
	}
#else
	if (NdotL > 0.0)
	{
		lighting = BRDF(L, V, N, geometric_normal, metallic, roughness, diffuse_color, light_color, F0, energy_compensation, attenuation, 1.0f, anisotropy, anisotropic_t, anisotropic_b, clear_coat, cc_roughness);
	}
#endif

	return lighting;
}

#ifndef COMPOSITION
vec3 DoFog(vec3 lighting, float time)
{
	float t2 = nextRand(payload.seed);
	t2 = -3.f * log2(1.f - t2);
	bool fog = false;
	if (t2 < time)
	{
		fog = true;
	}
	
	if ( fog && payload.depth == 0)
	{
		// fog - bounce in random sphere direction
		vec2 angle = vec2(nextRand(payload.seed), nextRand(payload.seed));
		angle.x *= 6.283185;
		angle.y = angle.y * 2. - 1.;//asin(angle.y*2.-1.); // I think this is right
		vec3 fog_r = vec2(sqrt(1.-angle.y*angle.y),angle.y).xyx * vec3(cos(angle.x),1,sin(angle.x));
		vec3 fog_pos = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * t2;
		vec3 fog_cont = TraceColorRay(fog_pos, fog_r, payload.seed, payload.depth + 1);
		//payload.color = lighting + (fog_cont * FOG_POWER);

		uint light_count = lights.lights[0].m_type >> 2;
		uint light_to_sample = min(int(nextRand(payload.seed) * light_count), light_count - 1);
		Light light = lights.lights[light_to_sample];
		
		vec3 L = mix(light.m_pos - fog_pos, light.m_dir, float(light.m_type == DIRECTIONAL_LIGHT));
		float light_dist = length(L);
		L /= light_dist;
		float max_light_dist = mix(light_dist, 100000, float(light.m_type == DIRECTIONAL_LIGHT));

		const vec3 light_color = light.m_color;
		const vec3 light_pos = light.m_pos;
		const vec3 light_dir = light.m_dir;
		const uint type = light.m_type & 3;
		const float inner_angle = light.m_inner_angle;
		const float outer_angle = light.m_outer_angle;

		const vec3 pos_to_light = light_pos - fog_pos;

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
			attenuation *= GetAngleAttenuation(light_dir, L, inner_angle, outer_angle);
		}

		vec3 shadow_dir = normalize(GetUniformHemisphereSample(payload.seed, L, light.m_physical_size));
		float shadow_mult = float(light_count) * float(!TraceShadowRay(fog_pos, shadow_dir, max_light_dist, payload.seed, payload.depth + 1));

		return lighting + (vec3(shadow_mult * (light_color * attenuation)) * FOG_POWER);
	}
	else
	{
		return lighting;
	}
}
#endif
