/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 *  Thanks to `https://github.com/PixarAnimationStudios/` for the packing code.
 */

#pragma once

#include <cstdint>
#include <cassert>
#include <cstring>

static inline const int max_vertex_count_limit = 256;
static inline const int primitive_packing_alignment = 1;
static inline const int max_primitive_count_limit = 256;
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

	//   m_x    | Bits | Content
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
		return Unpack(m_x, 8, 24) + 1;
	}

	void SetNumVertices(uint32_t num)
	{ 
		assert(num <= max_vertex_count_limit);
		m_x |= Pack(num - 1, 8, 24); 
    }

	std::uint32_t GetNumPrims() const {
		return Unpack(m_y, 8, 24) + 1;
	}

	void SetNumPrims(std::uint32_t num) {
		assert(num <= max_primitive_count_limit);
		m_y |= Pack(num - 1, 8, 24);
	}

	std::uint32_t GetPrimBegin() const {
		return Unpack(m_w, 20, 0) * primitive_packing_alignment;
	}

	void SetPrimBegin(std::uint32_t begin) {
		assert(begin % primitive_packing_alignment == 0);
		assert(begin / primitive_packing_alignment < ((1 << 20) - 1));
		m_w |= Pack(begin / primitive_packing_alignment, 20, 0);
	}

	std::uint32_t GetVertexBegin() const
	{
		return Unpack(m_z, 20, 0) * vertex_packing_alignment;
	}

	void SetVertexBegin(std::uint32_t begin)
	{
		assert(begin % vertex_packing_alignment == 0);
		assert(begin / vertex_packing_alignment < ((1 << 20) - 1));

		m_z |= Pack(begin / vertex_packing_alignment, 20, 0);
	}

	void SetBBox(uint8_t const bboxMin[3], uint8_t const bboxMax[3])
	{
		m_x |= Pack(bboxMin[0], 8, 0) | Pack(bboxMin[1], 8, 8) | Pack(bboxMin[2], 8, 16);

		m_y |= Pack(bboxMax[0], 8, 0) | Pack(bboxMax[1], 8, 8) | Pack(bboxMax[2], 8, 16);
	}

	void SetCone(int8_t coneOctX, int8_t coneOctY, int8_t minusSinAngle)
	{
		uint8_t anglebits = minusSinAngle;
		m_z |= Pack(coneOctX, 8, 20) | Pack((anglebits >> 0) & 0xF, 4, 28);
		m_w |= Pack(coneOctY, 8, 20) | Pack((anglebits >> 4) & 0xF, 4, 28);
	}

	void GetCone(int8_t& coneOctX, int8_t& coneOctY, int8_t& minusSinAngle) const
	{
		coneOctX = Unpack(m_z, 8, 20);
		coneOctY = Unpack(m_w, 8, 20);
		minusSinAngle = Unpack(m_z, 4, 28) | (Unpack(m_w, 4, 28) << 4);
	}

	void GetBBox(uint8_t bboxMin[3], uint8_t bboxMax[3]) const
	{
		bboxMin[0] = Unpack(m_x, 8, 0);
		bboxMin[0] = Unpack(m_x, 8, 8);
		bboxMin[0] = Unpack(m_x, 8, 16);

		bboxMax[0] = Unpack(m_y, 8, 0);
		bboxMax[0] = Unpack(m_y, 8, 8);
		bboxMax[0] = Unpack(m_y, 8, 16);
	}

	// https://github.com/PixarAnimationStudios/OpenSubdiv/blob/master/opensubdiv/far/patchParam.h#L234
	static std::uint32_t Pack(std::uint32_t value, int width, int offset)
	{
		return (std::uint32_t)((value & ((1 << width) - 1)) << offset);
	}

	// https://github.com/PixarAnimationStudios/OpenSubdiv/blob/master/opensubdiv/far/patchParam.h#L238
    static std::uint32_t Unpack(std::uint32_t value, int width, int offset)
	{
    	return (std::uint32_t)((value >> offset) & ((1 << width) - 1));
    }
};

class MeshletBuilder
{
	// todo
};
