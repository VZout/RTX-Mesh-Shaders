#define M_PI 3.1415926535897932384626433832795

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
    float alpha = roughness * roughness;
    float phi = 2.0 * M_PI * Xi.x + Random(normal.xz) * 0.1;
    //float phi = 2.f * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent space
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    // Convert to world Space
    return normalize(tangent * H.x + bitangent * H.y + normal * H.z);
}

// Normal distribution
float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(M_PI * denom * denom);
}

// Geometric Shadowing function
float G_SchlicksmithGGX(float NdotL, float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r*r) / 8.0f;
    float GL = NdotL / (NdotL * (1.0f - k) + k);
    float GV = NdotV / (NdotV * (1.0f - k) + k);
    return GL * GV;
}

// Fresnel function
vec3 F_Schlick(float cos_theta, float metallic, vec3 material_color)
{
    vec3 F0 = mix(vec3(0.04), material_color, metallic); // * material.specular
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
    return F;
}

// Fresnel function Roughness based
vec3 F_SchlickRoughness(float cos_theta, float metallic, vec3 material_color, float roughness)
{
    vec3 F0 = mix(vec3(0.04f), material_color, metallic); // * material.specular
    vec3 F = F0 + (max(vec3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0f - cos_theta, 5.0f);
    return F;
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 albedo, vec3 radiance)
{
    vec3 color = vec3(0);

    // Precalculate vectors and dot products
    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    if (dotNL > 0.0)
    {
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotNV, metallic, albedo);

        vec3 spec = (D * F * G) / (4.0 * dotNL * dotNV + 0.001f);

        vec3 kD = (vec3(1) - F) * (1.0 - metallic);

        color += (kD * albedo / M_PI + spec) * radiance * dotNL;
    }

    return color;
}