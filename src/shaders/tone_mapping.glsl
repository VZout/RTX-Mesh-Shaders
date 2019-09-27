vec3 saturate(vec3 i)
{
    return clamp(i, 0.f, 1.f);
}

vec3 ExposureCorrect(vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);
}

vec3 GammaCorrect(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

vec3 Linear(vec3 color)
{
    return saturate(color);
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESHighPerformance(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 ACESHighQuality(vec3 color)
{
    // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
    const mat3 ACESInputMat = mat3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );

    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    const mat3 ACESOutputMat = mat3(
         1.60475, -0.53108, -0.07367,
        -0.10208,  1.10813, -0.00605,
        -0.00327, -0.07276,  1.07602
    );

    color = color * ACESInputMat;

    // Apply RRT and ODT
    vec3 a = color * (color + 0.0245786f) - 0.000090537f;
    vec3 b = color * (0.983729f * color + 0.4329510f) + 0.238081f;
    color =  a / b;

    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    return clamp(color, 0.0, 1.0);
}

// Unreal 3, Documentation: "Color Grading"
// Gamma 2.2 correction is baked in, don't use with sRGB conversion!
vec3 Unreal3_Gamma(vec3 x)
{
    return x / (x + 0.155) * 1.019;
}

// Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
vec3 Lottes(vec3 x)
{
    const vec3 a = vec3(1.6);
    const vec3 d = vec3(0.977);
    const vec3 hdrMax = vec3(8.0);
    const vec3 midIn = vec3(0.18);
    const vec3 midOut = vec3(0.267);

    const vec3 b =
    (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const vec3 c =
    (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(x, a) / (pow(x, a * d) * b + c);
}

// Uchimura 2017, "HDR theory and practice"
// Math: https://www.desmos.com/calculator/gslcdxvipg
// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
vec3 Uchimura(vec3 x)
{
    const float P = 1.0;  // max display brightness
    const float a = 1.0;  // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4;  // linear section length
    const float c = 1.33; // black
    const float b = 0.0;  // pedestal

    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
    vec3 w2 = vec3(step(m + l0, x));
    vec3 w1 = vec3(1.0 - w0 - w2);

    vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
    vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
    vec3 L = vec3(m + a * (x - m));

    return T * w0 + L * w1 + S * w2;
}

// Filmic Tonemapping Operators http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Filmic(vec3 x) {
    vec3 X = max(vec3(0.0), x - 0.004);
    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, vec3(2.2));
}

// RomBinDaHouse
vec3 RomBinDaHouse(vec3 color)
{
    return exp(-1.0 / (2.72 * color + 0.15));
}

// Simple Reinhard `x/(1+x)`
vec3 Reinhard(vec3 color)
{
    return color / ( 1 + color);
}

// Luma based Reinhard
vec3 LumaBasedReinhard(vec3 color)
{
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma / (1. + luma);
    return color * toneMappedLuma / luma;
}

// White Preserving Luma based Reinhard
vec3 WhitePreservingLumaBasedReinhard(vec3 color)
{
    float white = 2.;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma * (1. + luma / (white * white)) / (1. + luma);
    return color * toneMappedLuma / luma;
}

// Haarm-Peter Duiker’s curve (no LUT) (Optimized by: Jim Hejl and Richard Burgess-Dawson)
// Note it has gamma baked in (not sure about the value)
vec3 HaarmPeterDuiker_Gamma(vec3 color)
{
    vec3 x = max(vec3(0), color - 0.004);
    return (x * (6.2 * x + .5)) / (x * (6.2 * x + 1.7) + 0.06);
}

vec3 Uncharted2_internal(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    const float W = 11.2;

    return (( x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2(vec3 color)
{
    const float W = 11.2;
    const float exposure_bias = 2.0f;

    vec3 curr = Uncharted2_internal(exposure_bias * color);
    vec3 white_scale = 1.0f / Uncharted2_internal(vec3(W));
    return curr * white_scale;
}