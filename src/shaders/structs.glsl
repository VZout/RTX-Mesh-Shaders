#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

#define MAX_RECURSION 3
#define EPSILON 0.001

struct Vertex
{
	float x;
	float y;
	float z;

	float u;
	float v;
	
	float nx;
	float ny;
	float nz;

	float tx;
	float ty;
	float tz;

	float bx;
	float by;
	float bz;
};

struct ReadableVertex
{
	vec3 pos;
	vec2 uv;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

struct Payload
{
	vec3 color;
	uint seed;
	uint depth;
};

struct RaytracingOffset
{   
	uint vertex_offset;
	uint idx_offset;
};

struct RaytracingMaterial
{   
	uint albedo_texture;
	uint normal_texture;
	uint roughness_texture;
	uint thickness_texture;

	vec4 color;
	float metallic;
	float roughness;
	float reflectivity;
	float transparency;
	float emissive;
	float normal_strength;
	float anisotropy;
	float anisotropy_x;
	float anisotropy_y;
	float clear_coat;
	float clear_coat_roughness;
	float u_scale;
	float v_scale;
	float two_sided;
	
	uint emissive_texture;
	float m_pad1;
};

struct Light
{
    vec3 m_pos;
    float m_radius;

	vec3 m_dir;
	float m_inner_angle;

    vec3 m_color;
    uint m_type;

	float m_outer_angle;
	float m_physical_size;
	vec3 m_padding;
};

#endif /* STRUCTS_GLSL */
