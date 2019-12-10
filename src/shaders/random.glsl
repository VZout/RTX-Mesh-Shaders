#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#include "pbr+_util.glsl"

// Initialize random seed
uint initRand(uint val0, uint val1)
{
	uint backoff = 16;
	uint v0 = val0, v1 = val1, s0 = 0;

	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Get 'random' value [0, 1]
float nextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

float Luminance(vec3 rgb)
{
    return dot(rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}

float ProbabilityToSampleDiffuse(vec3 difColor, vec3 specColor)
{
	float lumDiffuse = max(0.01f, Luminance(difColor.rgb));
	float lumSpecular = max(0.01f, Luminance(specColor.rgb));
	return lumDiffuse / (lumDiffuse + lumSpecular);
}

vec3 getPerpendicularVector(vec3 u)
{
	vec3 a = abs(u);
	uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
	uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, vec3(xm, ym, zm));
}

// When using this function to sample, the probability density is pdf = D * NdotH / (4 * HdotV)
vec3 getGGXMicrofacet(inout uint randSeed, float roughness, vec3 hitNorm)
{
	// Get our uniform random numbers
	vec2 randVal = vec2(nextRand(randSeed), nextRand(randSeed));

	// Get an orthonormal basis from the normal
	vec3 B = getPerpendicularVector(hitNorm);
	vec3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2 - 1.0) * randVal.x + 1)));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * M_PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) + B * (sinThetaH * sin(phiH)) + hitNorm * cosThetaH;
}

// Get a cosine-weighted random vector centered around a specified normal direction.
vec3 GetCosHemisphereSample(inout uint randSeed, vec3 hitNorm)
{
	// Get 2 random numbers to select our sample with
	vec2 randVal = vec2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	vec3 bitangent = getPerpendicularVector(hitNorm);
	vec3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(randVal.x);
	float phi = 2.0f * M_PI * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
}

vec3 GetUniformHemisphereSample(inout uint randSeed, vec3 hitNorm)
{
	// Get 2 random numbers to select our sample with
	vec2 randVal = vec2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	vec3 bitangent = getPerpendicularVector(hitNorm);
	vec3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(max(0.0f,1.0f - randVal.x*randVal.x));
	float phi = 2.0f * M_PI * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * randVal.x;
}

vec3 GetUniformHemisphereSample(inout uint randSeed, vec3 hitNorm, float angle)
{
	// Get 2 random numbers to select our sample with
	vec2 randVal = vec2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	vec3 bitangent = getPerpendicularVector(hitNorm);
	vec3 tangent = cross(bitangent, hitNorm);

	float r = sqrt(max(0.0f, 1.0f - randVal.x * randVal.x)) * sin(angle);
	float phi = 2.0f * M_PI * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * cos(asin(r));
}

#endif /* RANDOM_GLSL */
