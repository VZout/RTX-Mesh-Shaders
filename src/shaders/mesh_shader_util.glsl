#define USE_NATIVE   1
#extension GL_NV_mesh_shader : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#define GROUP_SIZE 32
#define NVMESHLET_VERTEX_COUNT      64
#define NVMESHLET_PRIMITIVE_COUNT   63	
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
	switch(n){
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
#if NVMESHLET_CLIP_Z_SIGNED // non vulkan pipe
	cullBits |= hPos.z < -hPos.w ? 16 : 0;
#else
	cullBits |= hPos.z <  0      ? 16 : 0;
#endif
	cullBits |= hPos.z >  hPos.w ? 32 : 0;
	cullBits |= hPos.w <= 0      ? 64 : 0; 
	
	return cullBits;
}

bool EarlyCull(uvec4 meshlet_desc, mat4 model, mat4 viewproj_mat)
{
	vec3 bbox_min;
	vec3 bbox_max;
	DecodeBbox(meshlet_desc, bbox_min, bbox_max);

	uint frustum_bits = ~0;

	for (int n = 0; n < 8; n++)
	{
		vec4 w_pos = model * GetBoxCorner(bbox_min, bbox_max, n);
		vec4 h_pos = viewproj_mat * w_pos;
		frustum_bits &= GetCullBits(h_pos);
	}

	return frustum_bits != 0;
}
