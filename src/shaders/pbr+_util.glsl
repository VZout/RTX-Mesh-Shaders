#define M_PI 3.1415926535897932384626433832795

#define MULTI_BOUNCE_AMBIENT_OCCLUSION 0

#define MEDIUMP_FLT_MIN    0.00006103515625
#define MEDIUMP_FLT_MAX    65504.0
#define FLT_EPS            1e-5
#define saturateMediump(x) x
#define PBR_PLUS
#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS 0.002025 // Only used for anisotropyic lobes
#define MIN_N_DOT_V 1e-4
#define ANISO
#define CLEAR_COAT
#define MATERIAL_HAS_NORMAL

float pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

vec3 ComputeDiffuseColor(const vec3 albedo, float metallic) {
    return albedo * (1.0 - metallic);
}

float PerceptualRoughnessToRoughness(float perceptual_roughness) {
    return perceptual_roughness * perceptual_roughness;
}

float ComputeDielectricF0(float reflectance) {
    return 0.16 * reflectance * reflectance;
}

vec3 ComputeF0(const vec3 albedo, float metallic, float reflectance) {
    return albedo * metallic + (reflectance * (1.0 - metallic));
}

vec3 SpecularDFG(vec3 F0, vec2 dfg)
{
    return mix(dfg.xxx, dfg.yyy, F0);
}

// TODO: Needs infestigating
float SingleBounceAO(float visibility) {
    #if MULTI_BOUNCE_AMBIENT_OCCLUSION == 1
    return 1.0;
    #else
    return visibility;
    #endif
}

vec3 GTAOMultiBounce(float visibility, const vec3 albedo) {
    // Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion"
    vec3 a =  2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c =  2.7552 * albedo + 0.6903;

    return max(vec3(visibility), ((visibility * a + b) * visibility + c) * visibility);
}

void MultiBounceAO(float visibility, const vec3 albedo, inout vec3 color) {
    #if MULTI_BOUNCE_AMBIENT_OCCLUSION == 1
    color *= GTAOMultiBounce(visibility, albedo);
    #endif
}

void MultiBounceSpecularAO(float visibility, const vec3 albedo, inout vec3 color) {
    #if MULTI_BOUNCE_AMBIENT_OCCLUSION == 1 && SPECULAR_AMBIENT_OCCLUSION == 1
    color *= GTAOMultiBounce(visibility, albedo);
    #endif
}

float GetSquareFalloffAttenuation(float distanceSquare, float falloff) {
    float factor = distanceSquare * falloff;
    float smoothFactor = clamp(1.0 - factor * factor, 0, 1);
    // We would normally divide by the square distance here
    // but we do it at the call site
    return smoothFactor * smoothFactor;
}

float GetDistanceAttenuation(const highp vec3 posToLight, float falloff) {
    float distanceSquare = dot(posToLight, posToLight);
    float attenuation = GetSquareFalloffAttenuation(distanceSquare, falloff);
    // Assume a punctual light occupies a volume of 1cm to avoid a division by 0
    return attenuation * 1.0 / max(distanceSquare, 1e-4);
}

float GetAngleAttenuation(const vec3 lightDir, const vec3 l, const float inner, const float outer) {
    float cosOuter = cos(outer);
    float spotScale = 1.0 / max(cos(inner) - cosOuter, 1e-4);
    float spotOffset = -cosOuter * spotScale;

    float cd = dot(lightDir, l);
    float attenuation = clamp(cd * spotScale + spotOffset, 0, 1);
    return attenuation * attenuation;
}

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float Random(vec2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 Hammersley2D(uint i, uint N)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 ImportanceSample_GGX(vec2 Xi, float roughness, vec3 normal)
{
    // Maps a 2D point to a hemisphere with spread based on roughness
    float alpha = roughness;
    //float phi = 2.0 * M_PI * Xi.x + Random(normal.xz) * 0.1;
    float phi = 2.f * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent space
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    // Convert to world Space
    return tangent * H.x + bitangent * H.y + normal * H.z;
}

// Normal distribution
float D_GGX(float NdotH, float roughness)
{
    float oneMinusNoHSquared = 1.0 - NdotH * NdotH;

    float a = NdotH * roughness;
    float k = roughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / M_PI);
    return saturateMediump(d);
}

// Normal distribution. Burley 2012, "Physically-Based Shading at Disney"
float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH) {

    // The values at and ab are perceptualRoughness^2, a2 is therefore perceptualRoughness^4
    // The dot product below computes perceptualRoughness^8. We cannot fit in fp16 without clamping
    // the roughness to too high values so we perform the dot product and the division in fp32
    float a2 = at * ab;
    highp vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float d2 = dot(d, d);
    float b2 = a2 / d2;
    return a2 * b2 * b2 * (1.0 / M_PI);
}

// Geometric Shadowing function
float G_SchlicksmithGGX(float NdotL, float NdotV, float roughness)
{
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = roughness * roughness;
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float lambdaV = NdotL * sqrt((NdotV - a2 * NdotV) * NdotV + a2);
    float lambdaL = NdotV * sqrt((NdotL - a2 * NdotL) * NdotL + a2);
    float v = 0.5 / (lambdaV + lambdaL);
    // a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
    // a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
    // clamp to the maximum value representable in mediump
    return saturateMediump(v);
}

// Geometric Shadowing function optimized (Perf > Quality)
float G_SchlicksmithGGXFast(float NdotL, float NdotV, float roughness)
{
    float v = 0.5 / mix(2.0 * NdotL * NdotV, NdotL + NdotV, roughness);
    return saturateMediump(v);
}

// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
float G_SmithGGXCorrelated_Anisotropic(float at, float ab, float TdotV, float BdotV,
        float TdotL, float BdotL, float NdotV, float NdotL) {
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float lambdaV = NdotL * length(vec3(at * TdotV, ab * BdotV, NdotV));
    float lambdaL = NdotV * length(vec3(at * TdotL, ab * BdotL, NdotL));
    float v = 0.5 / (lambdaV + lambdaL);
    return saturateMediump(v);
}

// Geometric Shadowing function Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
float G_Kelemen(float LdotH) {
    return saturateMediump(0.25 / (LdotH * LdotH));
}

// Geometric Shadowing function Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float G_Neubelt(float NdotV, float NdotL)
{
    return saturateMediump(1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV)));
}

// Fresnel function: Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
vec3 F_Schlick(const vec3 f0, float f90, float VdotH)
{
    return f0 + (f90 - f0) * pow5(1.0 - VdotH);
}
float F_Schlick(float f0, float f90, float VdotH)
{
    return f0 + (f90 - f0) * pow5(1.0 - VdotH);
}

float Fd_Lambert() {
    return 1.0 / M_PI;
}

float Fd_Burley(float roughness, float NdotV, float NdotL, float LdotH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = F_Schlick(1.0, f90, NdotL);
    float viewScatter  = F_Schlick(1.0, f90, NdotV);
    return lightScatter * viewScatter * (1.0 / M_PI);
}

float ClearCoatLobe(vec3 N, vec3 geometric_normal, vec3 H, float NdotH, float LdotH, float clear_coat, float cc_roughness, out float Fcc)
{
#ifdef MATERIAL_HAS_NORMAL
	float cc_NdotH = clamp(dot(geometric_normal, H), 0, 1);
#else
    float cc_NdotH = NdotH;
#endif
    float D = D_GGX(cc_NdotH, cc_roughness);
    float G = G_Kelemen(LdotH);
    float F = F_Schlick(0.04, 1.0, LdotH) * clear_coat; // fix IOR to 1.5

	Fcc = F;
	return D * G * F;
}

#ifdef ENV_SAMPLING
vec3 GetEnvReflection(vec3 R, float roughness)
{
    const float MAX_REFLECTION_LOD = 8.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    return textureLod(t_environment, R, lod).rgb;
}

vec3 GetEnvReflectionOffset(vec3 R, float roughness, float lod_offset)
{
    const float MAX_REFLECTION_LOD = 8.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    return textureLod(t_environment, R, lod + lod_offset).rgb;
}

void EvaluateClearCoatIBL(float clear_coat, float cc_roughness, float NdotV, vec3 R, float spec_ao, inout vec3 diff, inout vec3 spec)
{
    float cc_NdotV = NdotV;
    vec3 cc_R = R;

	 // The clear coat layer assumes an IOR of 1.5 (4% reflectance)
    float Fc = F_Schlick(0.04, 1.0, cc_NdotV) * clear_coat;
    float cc_attenuation = 1.0 - Fc;
    diff *= cc_attenuation;
    spec *= cc_attenuation;
    spec += GetEnvReflection(cc_R, cc_roughness) * (spec_ao * Fc);
}

void EvaluateSubsurfaceIBL(vec3 V, vec3 diffuseIrradiance, float perceptual_roughness, vec3 subsurface_color, float thickness, inout vec3 Fd, inout vec3 Fr) {
	float roughness = PerceptualRoughnessToRoughness(perceptual_roughness);
    vec3 viewIndependent = diffuseIrradiance;
    vec3 viewDependent = GetEnvReflectionOffset(-V, roughness, 1.0 + thickness);
    float attenuation = (1.0 - thickness) / (2.0 * M_PI);
    Fd += subsurface_color * (viewIndependent + viewDependent) * attenuation;
}
#endif

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 geometric_normal, float metallic, float perceptual_roughness, vec3 diffuse_color, vec3 radiance, vec3 F0, vec3 energy_compensation, float attenuation, float occlusion, float anisotropy, vec3 anisotropic_t, vec3 anisotropic_b, float clear_coat, float perceptual_cc_roughness)
{
    float roughness = PerceptualRoughnessToRoughness(perceptual_roughness);
	float cc_roughness = PerceptualRoughnessToRoughness(perceptual_cc_roughness);

    // Precalculate vectors and dot products
    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), MIN_N_DOT_V);
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);

    float F90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.f, 1.f);

#ifdef CLEAR_COAT
	float Fcc;
    float cc = ClearCoatLobe(N, geometric_normal, H, NdotH, LdotH, clear_coat, cc_roughness, Fcc);
    float cc_attenuation = 1.0 - Fcc;
#endif

#ifdef ANISO
    vec3 t = anisotropic_t;
    vec3 b = anisotropic_b;

    float TdotV = dot(t, V);
    float BdotV = dot(b, V);
    float TdotL = dot(t, L);
    float BdotL = dot(b, L);
    float TdotH = dot(t, H);
    float BdotH = dot(b, H);

	float at = max(roughness * (1.0 + anisotropy), MIN_ROUGHNESS);
    float ab = max(roughness * (1.0 - anisotropy), MIN_ROUGHNESS);

    float D = D_GGX_Anisotropic(at, ab, TdotH, BdotH, NdotH);
    float G = G_SmithGGXCorrelated_Anisotropic(at, ab, TdotV, BdotV, TdotL, BdotL, NdotV, NdotL);
    vec3 F = F_Schlick(F0, F90, LdotH);
	vec3 spec = (D * G) * F;
#else
    float D = D_GGX(NdotH, roughness);
    float G = G_SchlicksmithGGX(NdotL, NdotV, roughness);
    vec3 F = F_Schlick(F0, F90, LdotH);
	vec3 spec = (D * G) * F;
#endif
    //vec3 diff = diffuse_color * Fd_Lambert();
    vec3 diff = diffuse_color * Fd_Burley(roughness, NdotV, NdotL, LdotH);

#ifdef CLEAR_COAT
#ifdef MATERIAL_HAS_NORMAL
	vec3 color = (diff + spec * energy_compensation) * cc_attenuation * NdotL;
	float cc_NdotL = clamp(dot(geometric_normal, L), 0, 1);
	color += cc * cc_NdotL;

	// Return early so we don't apply NdotL twice.
	return (color * radiance) * (attenuation * occlusion);
#else
	vec3 color = (diff + spec * energy_compensation) * cc_attenuation + cc;
#endif
#else
    vec3 color = diff + spec * energy_compensation;
#endif

    return (color * radiance) * (NdotL * occlusion * attenuation);
}

vec3 SS_BRDF(vec3 L, vec3 V, vec3 N, vec3 geometric_normal, float metallic, float perceptual_roughness, vec3 diffuse_color, vec3 radiance, vec3 F0, vec3 energy_compensation, float attenuation, float occlusion,
		float thickness, vec3 subsurface_color, float subsurface_power)
{
	float roughness = PerceptualRoughnessToRoughness(perceptual_roughness);

    // Precalculate vectors and dot products
    vec3 H = normalize(V + L);
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
	float NdotV = max(dot(N, V), MIN_N_DOT_V);

    float F90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.f, 1.f);

	vec3 spec = vec3(0); 
	if (NdotL > 0.0) {
		float D = D_GGX(NdotH, roughness);
		float G = G_SchlicksmithGGX(NdotL, NdotV, roughness);
		vec3 F = F_Schlick(F0, F90, LdotH);
        spec = (D * G) * F * energy_compensation;
	}

	vec3 diff = diffuse_color * Fd_Burley(roughness, NdotV, NdotL, LdotH);

	// NoL does not apply to transmitted light
    vec3 color = (diff + spec) * (NdotL * occlusion);


	 // subsurface scattering
    // Use a spherical gaussian approximation of pow() for forwardScattering
    // We could include distortion by adding shading_normal * distortion to light.l
	float sss_scale = 1;
	float flt_power = 1;
    float scatterVoH = pow(clamp(dot(V, -L), 0, 1), flt_power) * sss_scale;
    float forwardScatter = exp2(scatterVoH * subsurface_power - subsurface_power);
    float backScatter = clamp(NdotL * thickness + (1.0 - thickness), 0, 1) * 0.5;
    float subsurface = mix(backScatter, 1.0, forwardScatter) * (1.0 - thickness);
    color += subsurface_color * (subsurface * Fd_Lambert());

	return (color * radiance) * attenuation;
}
