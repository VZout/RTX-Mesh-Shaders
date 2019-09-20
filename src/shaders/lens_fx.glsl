float linterp( float t ) {
    return clamp( 1.0 - abs( 2.0*t - 1.0 ), 0.f, 1.f );
}

float remap( float t, float a, float b ) {
    return clamp( (t - a) / (b - a), 0.f, 1.f );
}

vec4 spectrum_offset( float t ) {
    vec4 ret;
    float lo = step(t,0.5);
    float hi = 1.0-lo;
    float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
    ret = vec4(lo,1.0,hi, 1.) * vec4(1.0-w, w, 1.0-w, 1.);

    return pow( ret, vec4(1.0/2.2) );
}

vec3 Vignette(vec3 color, vec2 pos, float radius, float softness, float strength)
{
    float len = length(pos * 2 - 1);
    float vignette = smoothstep(radius, radius -  softness, len);
    return mix(color, color * vignette, strength);
}

vec2 ZoomUV(vec2 uv, float zoom)
{
    return (uv * zoom) + ((1 - (zoom)) / 2);
}

vec2 BarrelDistortUV(vec2 uv, float kcube)
{
    float k = -0.15;

    float r2 = (uv.x - 0.5) * (uv.x - 0.5) + (uv.y - 0.5) * (uv.y - 0.5);
    float f = 0;

    //only compute the cubic distortion if necessary
    if (kcube == 0.0)
    {
        f = 1 + r2 * k;
    }
    else
    {
        f = 1 + r2 * (k + kcube * sqrt(r2));
    };

    // get the right pixel for the current position
    float x = f * (uv.x - 0.5) + 0.5;
    float y = f * (uv.y - 0.5) + 0.5;

    return vec2(x, y);
}

vec2 BarrelDistortion_Internal(vec2 coord, float amt) {
    vec2 cc = coord - 0.5;
    float dist = dot(cc, cc);
    return coord + cc * dist * amt;
}

vec3 ChromaticAberration(vec2 uv, float strength, float zoom, vec2 resolution)
{
    const int num_iter = 11;
    const float reci_num_iter_f = 1.0 / float(num_iter);

    uv = ZoomUV(uv, zoom);
    vec4 sumcol = vec4(0.0);
    vec4 sumw = vec4(0.0);

    for (int i = 0; i < num_iter; ++i)
    {
        float t = float(i) * reci_num_iter_f;
        vec4 w = spectrum_offset(t);
        sumw += w;
        vec2 d_uv = (BarrelDistortion_Internal(uv, 0.6 * strength * t));
        d_uv = d_uv * resolution - 0.5f; // because we are not sampling.
        sumcol += w * imageLoad(t_input, ivec2(d_uv));
        //sumcol += w * SampleFXAA(tex, s, BarrelDistortion_Internal(uv, 0.6 * strength * t) * resolution, resolution);
    }

    return (sumcol / sumw).rgb;
}