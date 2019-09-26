#define M_PI 3.14159265358979

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
    vec3 F0 = mix(vec3(0.04, 0.04, 0.04), material_color, metallic); // * material.specular
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
    return F;
}

vec3 F_SchlickRoughness(float cos_theta, float metallic, vec3 material_color, float roughness)
{
    vec3 F0 = mix(vec3(0.04f, 0.04f, 0.04f), material_color, metallic); // * material.specular
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