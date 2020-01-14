/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <cstdint>
#include <cassert>
#include <cstring>

#include "vertex.hpp"
#include "util/bitfield.hpp"

static inline const int max_vertex_count_limit = 256;
static inline const int primitive_packing_alignment = 1;
static inline const int max_primitive_count_limit = 21;
static inline const std::uint32_t vertex_packing_alignment = 16;
static inline const std::uint32_t meshlets_per_task = 32;

template<typename T, typename A>
constexpr inline T SizeAlignTwoPower(T size, A alignment)
{
	return (size + (alignment - 1U)) & ~(alignment - 1U);
}

struct MeshletDesc
{
	MeshletDesc() : m_x(0), m_y(0), m_z(0), m_w(0)
	{
	}

	//   m_x        | Bits | Content
	//  ------------|:----:|----------------------------------------------
	//  bboxMinX    | 8    | bounding box coord relative to object bbox
	//  bboxMinY    | 8    | UNORM8
	//  bboxMinZ    | 8    |
	//  vertexMax   | 8    | number of vertex indices - 1 in the meshlet
	//  ------------|:----:|----------------------------------------------
	//   m_t    |      |
	//  ------------|:----:|----------------------------------------------
	//  bboxMaxX    | 8    | bounding box coord relative to object bbox
	//  bboxMaxY    | 8    | UNORM8
	//  bboxMaxZ    | 8    |
	//  primMax     | 8    | number of primitives - 1 in the meshlet
	//  ------------|:----:|----------------------------------------------
	//   m_z    |      |
	//  ------------|:----:|----------------------------------------------
	//  vertexBegin | 20   | offset to the first vertex index, times alignment
	//  coneOctX    | 8    | octant coordinate for cone normal, SNORM8
	//  coneAngleLo | 4    | lower 4 bits of -sin(cone.angle),  SNORM8
	//  ------------|:----:|----------------------------------------------
	//   m_w    |      |
	//  ------------|:----:|----------------------------------------------
	//  primBegin   | 20   | offset to the first primitive index, times alignment
	//  coneOctY    | 8    | octant coordinate for cone normal, SNORM8
	//  coneAngleHi | 4    | higher 4 bits of -sin(cone.angle), SNORM8
	//
	// Note : the bitfield is not expanded in the struct due to differences in how
	//        GPU & CPU compilers pack bit-fields and endian-ness.
	std::uint32_t m_x;
	std::uint32_t m_y;
	std::uint32_t m_z;
	std::uint32_t m_w;

	std::uint32_t GetNumVertices() const {
		return util::Unpack(m_x, 8, 24) + 1;
	}

	void SetNumVertices(uint32_t num)
	{ 
		assert(num <= max_vertex_count_limit);
		m_x |= util::Pack(num - 1, 8, 24);
    }

	std::uint32_t GetNumPrims() const {
		return util::Unpack(m_y, 8, 24) + 1;
	}

	void SetNumPrims(std::uint32_t num) {
		assert(num <= max_primitive_count_limit);
		m_y |= util::Pack(num - 1, 8, 24);
	}

	std::uint32_t GetPrimBegin() const {
		return util::Unpack(m_w, 20, 0) * primitive_packing_alignment;
	}

	void SetPrimBegin(std::uint32_t begin) {
		assert(begin % primitive_packing_alignment == 0);
		assert(begin / primitive_packing_alignment < ((1 << 20) - 1));
		m_w |= util::Pack(begin / primitive_packing_alignment, 20, 0);
	}

	std::uint32_t GetVertexBegin() const
	{
		return util::Unpack(m_z, 20, 0) * vertex_packing_alignment;
	}

	void SetVertexBegin(std::uint32_t begin)
	{
		assert(begin % vertex_packing_alignment == 0);
		assert(begin / vertex_packing_alignment < ((1 << 20) - 1));

		m_z |= util::Pack(begin / vertex_packing_alignment, 20, 0);
	}

	void SetBBox(uint8_t const bboxMin[3], uint8_t const bboxMax[3])
	{
		m_x |= util::Pack(bboxMin[0], 8, 0) | util::Pack(bboxMin[1], 8, 8) | util::Pack(bboxMin[2], 8, 16);
		m_y |= util::Pack(bboxMax[0], 8, 0) | util::Pack(bboxMax[1], 8, 8) | util::Pack(bboxMax[2], 8, 16);
	}

	void SetCone(int8_t coneOctX, int8_t coneOctY, int8_t minusSinAngle)
	{
		uint8_t anglebits = minusSinAngle;
		m_z |= util::Pack(coneOctX, 8, 20) | util::Pack((anglebits >> 0) & 0xF, 4, 28);
		m_w |= util::Pack(coneOctY, 8, 20) | util::Pack((anglebits >> 4) & 0xF, 4, 28);
	}

	void GetCone(int8_t& coneOctX, int8_t& coneOctY, int8_t& minusSinAngle) const
	{
		coneOctX = util::Unpack(m_z, 8, 20);
		coneOctY = util::Unpack(m_w, 8, 20);
		minusSinAngle = util::Unpack(m_z, 4, 28) | (util::Unpack(m_w, 4, 28) << 4);
	}

	void GetBBox(uint8_t min[3], uint8_t max[3]) const
	{
		min[0] = util::Unpack(m_x, 8, 0);
		min[1] = util::Unpack(m_x, 8, 8);
		min[2] = util::Unpack(m_x, 8, 16);

		max[0] = util::Unpack(m_y, 8, 0);
		max[1] = util::Unpack(m_y, 8, 8);
		max[2] = util::Unpack(m_y, 8, 16);
	}

};

struct MeshBoundingBox
{
	glm::vec3 m_average_normal = glm::vec3(0);
	std::vector<glm::vec3> m_tri_normals;
	glm::vec3 m_min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 m_max = glm::vec3(-std::numeric_limits<float>::max());
};

struct MeshletBuilder
{
	template<typename VT>
	static MeshBoundingBox CalculateBoundingBox(MeshData const & mesh_data)
	{
		MeshBoundingBox bbox = {};

		for (auto i = 0; i < mesh_data.m_num_indices; i += 3)
		{
			glm::ivec3 triangle;
			memcpy(&triangle, &mesh_data.m_indices[i * mesh_data.m_indices_stride], mesh_data.m_indices_stride * 3);

			auto v0 = mesh_data.m_positions[triangle[0]];
			auto v1 = mesh_data.m_positions[triangle[1]];
			auto v2 = mesh_data.m_positions[triangle[2]];

			// bounding box
			{
				bbox.m_min = glm::min(bbox.m_min, v0);
				bbox.m_min = glm::min(bbox.m_min, v1);
				bbox.m_min = glm::min(bbox.m_min, v2);

				bbox.m_max = glm::max(bbox.m_max, v0);
				bbox.m_max = glm::max(bbox.m_max, v1);
				bbox.m_max = glm::max(bbox.m_max, v2);
			}

			// cone
			{
				glm::vec3 cross = glm::cross(v1 - v0, v2 - v0);
				float length = glm::length(cross);

				glm::vec3 normal;
				if (length > FLT_EPSILON)
				{
					normal = cross * (1.0f / length);
				}
				else
				{
					normal = cross;
				}

				bbox.m_average_normal += normal;
				bbox.m_tri_normals.push_back(normal);
			}
		}

		// potential improvement, instead of average maybe use
		// http://www.cs.technion.ac.il/~cggc/files/gallery-pdfs/Barequet-1.pdf
		float len = glm::length(bbox.m_average_normal);
		if (len > FLT_EPSILON)
		{
			bbox.m_average_normal = bbox.m_average_normal / len;
		}
		else
		{
			bbox.m_average_normal = glm::vec3(0.0f);
		}

		return bbox;
	}

	static void TruncateBBoxToMeshBBox(glm::vec3& bbox_min, glm::vec3& bbox_max, MeshBoundingBox const& mesh_bbox)
	{
		glm::vec3 object_bbox_extent = mesh_bbox.m_max - mesh_bbox.m_min;
		bbox_min = bbox_min - mesh_bbox.m_min;
		bbox_max = bbox_max - mesh_bbox.m_min;
		bbox_min = bbox_min / object_bbox_extent;
		bbox_max = bbox_max / object_bbox_extent;
	}
};
