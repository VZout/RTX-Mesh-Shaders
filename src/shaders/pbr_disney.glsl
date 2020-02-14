#ifndef PBR_DISNEY_GLSL
#define PBR_DISNEY_GLSL

#define PI       3.1415926535897932384626433832795
#define HALF_PI  1.5707963267948966192313216916398
#define TWO_PI   6.283185307179586476925286766559
#define INV_PI   0.3183098861837907

// wo = V
// wi = L

float PowerHeuristic(float a, float b)
{
    float t = a * a;
    return t / (b * b + t);
}

float pow2(float x) { 
    return x*x;
}

float GTR1(float NdotH, float a)
{
    if (a >= 1.0) return INV_PI;
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

float GTR2(float NdotH, float a)
{
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return a2 / (PI * t * t);
}

float GTR2_Aniso(float NdotH, float HdotX, float HdotY, float ax, float ay) {
    return 1. / (PI * ax*ay * pow2( pow2(HdotX/ax) + pow2(HdotY/ay) + NdotH*NdotH ));
}

float SmithGGX_G(float NdotV, float a)
{
    float a2 = a * a;
    float b = NdotV * NdotV;
    return 1.0 / (NdotV + sqrt(a2 + b - a2 * b));
}

bool SameHemiSphere(const in vec3 V, const in vec3 L, const in vec3 N)
{
    return dot(V, N) * dot(L, N) > 0.0;
}

// Can be heavily optimized
float SmithG_GGX_Aniso(float NdotV, float VdotX, float VdotY, float ax, float ay) {
    return 1. / (NdotV + sqrt(pow2(VdotX * ax) + pow2(VdotY * ay) + pow2(NdotV) ));
}

float SchlickFresnelReflectance(float u)
{
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m;
}

void DirectionOfAnisotropicity(vec3 normal, inout Surface surface){
    surface.aniso_t = cross(normal, vec3(1.0, 0.0, 1.0));
    surface.aniso_b = normalize(cross(normal, surface.aniso_t));
    surface.aniso_t = normalize(cross(normal, surface.aniso_b));
}

vec3 CosineSampleHemisphere(float u1, float u2) {
  vec3 dir;
  float r = sqrt(u1);
  float phi = TWO_PI * u2;
  dir.x = r * cos(phi);
  dir.y = r * sin(phi);
  dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));
  return dir;
}

void CalculateCSW(inout Surface surface)
{
    vec3 specular_tint = vec3(0);

    const vec3 cd_lin = surface.albedo;
    const float cd_lum = dot(cd_lin, vec3(0.3, 0.6, 0.1));
    const vec3 c_tint = cd_lum > 0.0 ? (cd_lin / cd_lum) : vec3(1);
    const vec3 c_spec0 = mix((1.0 - surface.specular * 0.3) * mix(vec3(1), c_tint, specular_tint), cd_lin, surface.metallic);
    const float cs_lum = dot(c_spec0, vec3(0.3, 0.6, 0.1));
    const float cs_w = cs_lum / (cs_lum + (1.0 - surface.metallic) * cd_lum);
    surface.csw = cs_w;
}

float PDFDisneyLambartianReflection(vec3 V, vec3 L, vec3 N)
{
	return SameHemiSphere(V, L, N) ? abs(dot(N, L)) / PI : 0.f;
}

float PDFDisneyMicrofacet(Surface surface, vec3 V, vec3 L, vec3 N) {
    if (!SameHemiSphere(V, L, N)) return 0.0f;
	vec3 wh = normalize(V + L);
    
    float NdotH = dot(N, wh);
    float alpha2 = surface.roughness * surface.roughness;
    alpha2 *= alpha2;
    
    float cos2Theta = NdotH * NdotH;
    float denom = cos2Theta * (alpha2 - 1.0f) + 1.0f;

    if(denom == 0.0f) 
		return 0.0f;

    float pdfDistribution = alpha2 * NdotH / (PI * denom * denom);
    return pdfDistribution / (4.0f * dot(V, wh));
}

float PDFDisneyMicrofacetAniso(Surface surface, vec3 V, vec3 L, vec3 N) {
    if (!SameHemiSphere(V, L, N)) return 0.0f;
	vec3 wh = normalize(V + L);

	float aspect = sqrt(1.0 - surface.anisotropic * 0.9);
    float alphax = max(0.001, pow2(surface.roughness) / aspect);
    float alphay = max(0.001, pow2(surface.roughness) * aspect);

	float alphax2 = alphax * alphax;
    float alphay2 = alphax * alphay;

    float HdotX = dot(wh, surface.aniso_t);
    float HdotY = dot(wh, surface.aniso_b);
	float NdotH = dot(N, wh);
    
    float denom = HdotX * HdotX / alphax2 + HdotY * HdotY / alphay2 + NdotH * NdotH;

    if(denom == 0.0f) 
		return 0.0f;

    float pdfDistribution = NdotH /(PI * alphax * alphay * denom * denom);
    return pdfDistribution / (4.f * dot(V, wh));
}

float PDFDisneyClearCoat(Surface surface, vec3 V, vec3 L, vec3 N) {
    if (!SameHemiSphere(V, L, N)) return 0.0f;

    vec3 wh = V + L;
    wh = normalize(wh);
	
    float NdotH = abs(dot(wh, N));
    float Dr = GTR1(NdotH, mix(0.1, 0.001, surface.clearcoat_gloss));
    return Dr * NdotH / (4. * abs(dot(wh, V)));
}

float DisneyPDF(Surface surface, vec3 V, vec3 L, vec3 N)
{
	float pdf_diff = PDFDisneyLambartianReflection(V, L, N);
	float pdf_micro = PDFDisneyMicrofacetAniso(surface, V, L, N);
	float pdf_cc = PDFDisneyClearCoat(surface, V, L, N);

	//return (pdf_diff + pdf_micro + pdf_cc) / 3.0f;
	return pdf_cc * surface.clearcoat + (1.f - surface.clearcoat) * (surface.csw * pdf_micro + (1.f - surface.csw) * pdf_diff);
}

vec3 DisneyDiffuse(Surface surface, float NdotL, float NdotV, float LdotH)
{
    return /*Fd_Burley(surface.roughness, NdotV, NdotL, LdotH)*/ Fd_Lambert() * surface.albedo;
}

vec3 DisneMicrofacetIsotropic(Surface surface, float NdotL, float NdotV, float NdotH, float LdotH)
{
    vec3 specular_tint = vec3(1);
	float Cdlum = Luminance(surface.albedo);

	vec3 Ctint = Cdlum > 0.0 ? surface.albedo/Cdlum : vec3(1.0); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(surface.specular * 0.08 * mix(vec3(1.0), Ctint, specular_tint), surface.albedo, surface.metallic); // TODO: This is f0?
    
    float a = max(.001, pow2(surface.roughness));// TODO: Replace pow2 with roughness to perceptual roughness
    float Ds = GTR2(NdotH, a);
    float FH = SchlickFresnelReflectance(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1), FH);
    float Gs = G_SchlicksmithGGX(NdotL, NdotV, a);
    
    return Gs * Fs * Ds;
}

vec3 DisneMicrofacetAnisotropic(Surface surface, float NdotL, float NdotV, float NdotH, float LdotH, vec3 V, vec3 L, vec3 H)
{
    vec3 specular_tint = vec3(1);
	float Cdlum = Luminance(surface.albedo);

	vec3 Ctint = Cdlum > 0.0 ? surface.albedo / Cdlum : vec3(1.0); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(surface.specular * 0.08 * mix(vec3(1.0), Ctint, specular_tint), surface.albedo, surface.metallic); // TODO: This is f0?
    
    float aspect = sqrt(1.0 - surface.anisotropic * 0.9);
	float ax = max(.001, pow2(surface.roughness) / aspect);
    float ay = max(.001, pow2(surface.roughness) * aspect);

	float Ds = GTR2_Aniso(NdotH, dot(H, surface.aniso_t), dot(H, surface.aniso_b), ax, ay);
	float FH = SchlickFresnelReflectance(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1), FH);
	float Gs = SmithG_GGX_Aniso(NdotL, dot(L, surface.aniso_t), dot(L, surface.aniso_b), ax, ay);
    Gs *= SmithG_GGX_Aniso(NdotV, dot(V, surface.aniso_t), dot(V, surface.aniso_b), ax, ay);
    
    return Gs * Fs * Ds;
}

float DisneyClearCoat(Surface surface, float NdotL, float NdotV, float NdotH, float LdotH) {
    float clear_coat_boost = 1.f; // Default: 0.25f

    float gloss = mix(0.1f, 0.001f, surface.clearcoat_gloss);
    float Dr = GTR1(abs(NdotH), gloss); // TODO: Validate GTR1
    float FH = SchlickFresnelReflectance(LdotH);
    float Fr = mix(0.04f, 1.f, FH);
    float Gr = G_SchlicksmithGGX(NdotL, NdotV, 0.25); //TODO: Validate this.
    return clear_coat_boost * surface.clearcoat * Fr * Gr * Dr;
}

// Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
// 1.25 scale is used to (roughly) preserve albedo
// fss90 used to "flatten" retroreflection based on roughness
vec3 DisneySubsurface(Surface surface, const in float NdotL, const in float NdotV, const in float LdotH)
{
	vec3 subsurface_color = surface.albedo;
    float FL = SchlickFresnelReflectance(NdotL), FV = SchlickFresnelReflectance(NdotV);
    float Fss90 = LdotH * LdotH * surface.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.f / (NdotL + NdotV) - 0.5f) + 0.5f);
    
    return Fd_Lambert() * ss * subsurface_color;
}

vec3 DisneySheen(Surface surface, float LdotH)
{
	vec3 sheen_tint = vec3(1);

    float FH = SchlickFresnelReflectance(LdotH);
    float Cdlum = Luminance(surface.albedo);

    vec3 Ctint = Cdlum > 0.0 ? surface.albedo / Cdlum : vec3(1.0);
    vec3 Csheen = mix(vec3(1.0), Ctint, sheen_tint);
    vec3 Fsheen = FH * surface.sheen * Csheen;
    return FH * surface.sheen * Csheen;
}

vec3 DisneyEval(Surface surface, in float NdotL, in const float NdotV, in const float NdotH, const float LdotH, vec3 V, vec3 L, vec3 H, float occlusion)
{
	const vec3 diffuse = DisneyDiffuse(surface, NdotL, NdotV, LdotH) * occlusion;
    const float clear_coat = DisneyClearCoat(surface, NdotL, NdotV, NdotH, LdotH)  * occlusion;
	const vec3 sub_surface = DisneySubsurface(surface, NdotL, NdotV, LdotH);
	const vec3 sheen = DisneySheen(surface, LdotH)  * occlusion;
	//const vec3 specular = DisneMicrofacetIsotropic(surface, NdotL, NdotV, NdotH, LdotH);
	const vec3 specular = DisneMicrofacetAnisotropic(surface, NdotL, NdotV, NdotH, LdotH, V, L, H) * occlusion;

	//if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0); // This fucks subsurface of course.

    const vec3 f = (mix(diffuse, sub_surface, surface.thickness) + sheen)
        * (1.0 - surface.metallic)
        + specular
        + clear_coat;
    return f * NdotL;
}


void CreateBasis(vec3 normal, out vec3 tangent, out vec3 binormal){
    if (abs(normal.x) > abs(normal.y)) {
        tangent = normalize(vec3(0., normal.z, -normal.y));
    }
    else {
        tangent = normalize(vec3(-normal.z, 0., normal.x));
    }
    binormal = cross(normal, tangent);
}

vec3 SphericalDirection(float sinTheta, float cosTheta, float sinPhi, float cosPhi) {
    return normalize(vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));
}

vec3 DisneyClearCoatSample(Surface surface, vec3 V, vec3 N, vec2 u) {
	float gloss = mix(0.1, 0.001, surface.clearcoat_gloss);
    float alpha2 = gloss * gloss;
    float cosTheta = sqrt((1. - pow(alpha2, 1. - u[1])) / (1. - alpha2));
    float sinTheta = sqrt(max(0.0, 1.0 - (cosTheta * cosTheta)));
    float phi = TWO_PI * u[0];
    
    vec3 H = normalize(SphericalDirection(sinTheta, cosTheta, sin(phi), cos(phi)));
     
    vec3 tangent = vec3(0.f), binormal = vec3(0.f);
    CreateBasis(N, tangent, binormal);
    
    H = H.x * tangent + H.y * binormal + H.z * N;
    
    if(!SameHemiSphere(V, H, N)) {
       H *= -1.;
    }
            
    return reflect(-V, H);   
}

// Microfacet Anisotropic Sample
vec3 DisneyAnisotropicSpecularSample(Surface surface, vec3 V, vec3 N, vec2 u) {
	float cosTheta = 0., phi = 0.;
    
    float aspect = sqrt(1. - surface.anisotropic * 0.9);
    float alphax = max(.001, pow2(surface.roughness)/aspect);
    float alphay = max(.001, pow2(surface.roughness)*aspect);
    
    phi = atan(alphay / alphax * tan(2. * PI * u[0] + .5 * PI)); // Is this correct? Should this not be sqrt()... 
    
    if (u[0] > .5f) phi += PI;
    float sinPhi = sin(phi), cosPhi = cos(phi);
    float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
    float alpha2 = 1. / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
    float tanTheta2 = alpha2 * u[1] / (1. - u[1]);
    cosTheta = 1. / sqrt(1. + tanTheta2);
    
    float sinTheta = sqrt(max(0., 1. - cosTheta * cosTheta));
    vec3 H = SphericalDirection(sinTheta, cosTheta, sin(phi), cos(phi));
         
    H = H.x * surface.aniso_t + H.y * surface.aniso_b + H.z * N;
    
    if(!SameHemiSphere(V, H, N)) {
       H *= -1.;
    }
            
    return reflect(-V, H);
}

vec3 DisneyDiffuseSample(Surface surface, vec3 V, vec3 N, vec2 u) {
	vec3 H = CosineSampleHemisphere(u.x, u.y);
	vec3 tangent = vec3(0.f), binormal = vec3(0.f);
	CreateBasis(N, tangent, binormal);

	H = H.x * tangent + H.y * binormal + H.z * N;

    if (dot(V, N) < 0.) 
		H.z *= -1.;

	return H;
}

vec3 DisneySample(Surface surface, inout uint seed, in const vec3 V, in const vec3 N) {
    float r1 = nextRand(seed);
    float r2 = nextRand(seed);
	float rnd = nextRand(seed);

    const vec3 U = abs(N.z) < (1.0 - EPSILON) ? vec3(0, 0, 1) : vec3(1, 0, 0);
    const vec3 T = normalize(cross(U, N));
    const vec3 B = cross(N, T);
	
	/*
    // clearcoat
    if (rnd <= 0.3333)
    {
		return DisneyClearCoatSample(surface, V, N, vec2(r1, r2));
    }

    // specular
    if (rnd >= 0.3333 && rnd < 0.6666)
    {
        return DisneyAnisotropicSpecularSample(surface, V, N, vec2(r1, r2));
    }

    // diffuse
	return DisneyDiffuseSample(surface, V, N, vec2(r1, r2));
	*/

    // clearcoat
    if (r1 < surface.clearcoat)
    {
		r1 /= surface.clearcoat;

		return DisneyClearCoatSample(surface, V, N, vec2(r1, r2));

		const float a = mix(0.1, 0.001, surface.clearcoat_gloss);
		const float cosTheta = sqrt((1.0 - pow(a*a, 1.0 - r2)) / (1.0 - a*a));
		const float sinTheta = sqrt(max(0.0, 1.0 - (cosTheta * cosTheta)));
		const float phi = r1 * TWO_PI;
		vec3 H = normalize(vec3(
		  cos(phi) * sinTheta,
		  sin(phi) * sinTheta,
		  cosTheta
		));
		H = H.x * T + H.y * B + H.z * N;
		if (dot(H, V) <= 0.0) H = H * -1.0;
		return reflect(-V, H);
    }
	else
	{
	    r1 -= (surface.clearcoat);
       	r1 /= (1.f - surface.clearcoat);

		// specular
		if (r2 < surface.csw)
        {
            r2 /= surface.csw;

			return DisneyAnisotropicSpecularSample(surface, V, N, vec2(r1, r2));

			const float a = max(0.001, surface.roughness);
			const float cosTheta = sqrt((1.0 - r2) / (1.0 + (a*a-1.0) * r2));
			const float sinTheta = sqrt(max(0.0, 1.0 - (cosTheta * cosTheta)));
			const float phi = r1 * TWO_PI;
			vec3 H = normalize(vec3(
			  cos(phi) * sinTheta,
			  sin(phi) * sinTheta,
			  cosTheta
			));
			H = H.x * T + H.y * B + H.z * N;
			if (dot(H, V) <= 0.0) H = H * -1.0;
			return reflect(-V, H);
		}
		// diffuse
		else
		{
			r2 -= surface.csw;
            r2 /= (1.f - surface.csw);

			return DisneyDiffuseSample(surface, V, N, vec2(r1, r2));
			
			const vec3 H = CosineSampleHemisphere(r1, r2);
			return T * H.x + B * H.y + N * H.z;
		}
	}
}

#endif /* PBR_DISNEY_GLSL */
