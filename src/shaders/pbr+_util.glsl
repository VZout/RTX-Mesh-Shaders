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


// Fresnel function Roughness based
vec3 F_SchlickRoughness(float cos_theta, float metallic, vec3 material_color, float roughness)
{
    vec3 F0 = mix(vec3(0.04f), material_color, metallic); // * material.specular
    vec3 F = F0 + (max(vec3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0f - cos_theta, 5.0f);
    return F;
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

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float perceptual_roughness, vec3 diffuse_color, vec3 radiance, vec3 F0, vec3 energy_compensation, float occlusion)
{
    vec3 color = vec3(0);

    float roughness = PerceptualRoughnessToRoughness(perceptual_roughness);

    // Precalculate vectors and dot products
    vec3 H = normalize(V + L);
    float dotNV = max(dot(N, V), MIN_N_DOT_V);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);

    float F90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.f, 1.f);

    // D = Normal distribution (Distribution of the microfacets)
    float D = D_GGX(dotNH, roughness);
    // G = Geometric shadowing term (Microfacets shadowing)
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
    // F = Fresnel factor (Reflectance depending on angle of incidence)
    vec3 F = F_Schlick(F0, F90, dotLH);

    vec3 spec = (D * G) * F;
    vec3 diff = diffuse_color * Fd_Lambert();
    //vec3 diff = diffuse_color * Fd_Burley(roughness, dotNV, dotNL, dotLH);

    color = diff + spec * energy_compensation;

    return (color * radiance) * dotNL * occlusion;
}
