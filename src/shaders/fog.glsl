float FogFactorLinear(const float dist, const float start, const float end)
{
  return 1.0 - clamp((end - dist) / (end - start), 0.0, 1.0);
}
