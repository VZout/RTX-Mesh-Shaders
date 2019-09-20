vec3 saturate(vec3 i)
{
    return clamp(i, 0.f, 1.f);
}

vec3 ExposureCorrect(vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);;
}

vec3 GammaCorrect(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFittedHighPerformance(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 ACESFittedHighQuality(vec3 color)
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