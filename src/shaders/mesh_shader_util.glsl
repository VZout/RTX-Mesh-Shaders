#define USE_NATIVE   1
#extension GL_NV_mesh_shader : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#define GROUP_SIZE 32
#define NVMESHLET_VERTEX_COUNT      64
#define NVMESHLET_PRIMITIVE_COUNT   126
#define NVMESHLET_PRIM_ALIGNMENT        1
#define NVMESHLET_VERTEX_ALIGNMENT      16

#define NVMSH_INDEX_BITS      8
#define NVMSH_PACKED4X8_GET(packed, idx)   (((packed) >> (NVMSH_INDEX_BITS * (idx))) & 255)

void DecodeMeshlet(uvec4 meshlet_desc, out uint vert_max, out uint prim_max, out uint vert_begin, out uint prim_begin)
{
	vert_begin = (meshlet_desc.z & 0xFFFFF) * NVMESHLET_VERTEX_ALIGNMENT;
	prim_begin = (meshlet_desc.w & 0xFFFFF) * NVMESHLET_PRIM_ALIGNMENT;
	vert_max = (meshlet_desc.x >> 24);
	prim_max = (meshlet_desc.y >> 24);
}

void DecodeBbox(uvec4 meshlet_desc, out vec3 oBboxMin, out vec3 oBboxMax)
{
#ifdef TASK
	vec3 bbox_min = unpackUnorm4x8(meshlet_desc.x).xyz;
	vec3 bbox_max = unpackUnorm4x8(meshlet_desc.y).xyz;
  
	vec3 object_bbox_extent = drawcall_info.object_bbox_max.xyz - drawcall_info.object_bbox_min.xyz;

	oBboxMin = bbox_min * object_bbox_extent + drawcall_info.object_bbox_min.xyz;
	oBboxMax = bbox_max * object_bbox_extent + drawcall_info.object_bbox_min.xyz;
#endif
}

vec4 GetBoxCorner(vec3 bboxMin, vec3 bboxMax, int n)
{
	switch(n)
	{
    case 0:
		return vec4(bboxMin.x,bboxMin.y,bboxMin.z,1);
    case 1:
		return vec4(bboxMax.x,bboxMin.y,bboxMin.z,1);
    case 2:
		return vec4(bboxMin.x,bboxMax.y,bboxMin.z,1);
    case 3:
		return vec4(bboxMax.x,bboxMax.y,bboxMin.z,1);
    case 4:
		return vec4(bboxMin.x,bboxMin.y,bboxMax.z,1);
    case 5:
		return vec4(bboxMax.x,bboxMin.y,bboxMax.z,1);
    case 6:
		return vec4(bboxMin.x,bboxMax.y,bboxMax.z,1);
    case 7:
		return vec4(bboxMax.x,bboxMax.y,bboxMax.z,1);
	}
}

uint GetCullBits(vec4 hPos)
{
	uint cullBits = 0;
	cullBits |= hPos.x < -hPos.w ?  1 : 0;
	cullBits |= hPos.x >  hPos.w ?  2 : 0;
	cullBits |= hPos.y < -hPos.w ?  4 : 0;
	cullBits |= hPos.y >  hPos.w ?  8 : 0;
	cullBits |= hPos.z <  0      ? 16 : 0;
	cullBits |= hPos.z >  hPos.w ? 32 : 0;
	cullBits |= hPos.w <= 0      ? 64 : 0; 
	
	return cullBits;
}

void PixelBBoxEpsilon(inout vec2 pixelMin, inout vec2 pixelMax)
{
  // Apply some safety around the bbox to take into account fixed point rasterization.
  // This logic will only work without MSAA active.
  const float epsilon = (1.0 / 256.0);
  pixelMin -= epsilon;
  pixelMax += epsilon;
}

// oct_ code from "A Survey of Efficient Representations for Independent Unit Vectors"
// http://jcgt.org/published/0003/02/01/paper.pdf
vec2 oct_signNotZero(vec2 v) {
	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

vec3 oct_to_vec3(vec2 e) {
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * oct_signNotZero(v.xy);
  
	return normalize(v);
}

void DecodeNormalAngle(uvec4 meshlet_desc, out vec3 normal, out float angle)
{
  uint packed_vec =  (((meshlet_desc.z >> 20) & 0xFF) << 0)  |
                     (((meshlet_desc.w >> 20) & 0xFF) << 8)  |
                     (((meshlet_desc.z >> 28)       ) << 16) |
                     (((meshlet_desc.w >> 28)       ) << 20);

  vec3 unpacked_vec = unpackSnorm4x8(packed_vec).xyz;
  
  float winding = 1.f;
  normal = oct_to_vec3(unpacked_vec.xy) * winding;
  angle = unpacked_vec.z;
}

bool EarlyCull(uvec4 meshlet_desc, mat4 model, vec3 view_pos, mat4 viewproj_mat)
{
	vec3 bbox_min;
	vec3 bbox_max;
	DecodeBbox(meshlet_desc, bbox_min, bbox_max);

	uint frustum_bits = ~0;
	bool backface = false;

	// Early backface culling
	vec3 o_group_normal;
	float angle;
	DecodeNormalAngle(meshlet_desc, o_group_normal, angle);
	vec3 w_group_normal = normalize(inverse(transpose(mat3(model))) * o_group_normal);
	backface = angle < 0;

	for (int n = 0; n < 8; n++)
	{
		// Early frustum culling
		vec4 w_pos = model * GetBoxCorner(bbox_min, bbox_max, n);
		vec4 h_pos = viewproj_mat * w_pos;
		frustum_bits &= GetCullBits(h_pos);

		// Early backface culling
		vec3 w_dir = normalize(view_pos - w_pos.xyz);
   		backface = backface && (dot(w_group_normal, w_dir) < angle);
	}

	return (frustum_bits != 0 || backface);
}
