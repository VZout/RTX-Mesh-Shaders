#define FOG_COLOR vec3(0, 0, 0)

#define FOG_POWER 0.1f

float FogFactorLinear(const float dist, const float start, const float end)
{
  return 1.0 - clamp((end - dist) / (end - start), 0.0, 1.0);
}
