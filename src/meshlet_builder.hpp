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
	MeshletDesc()
	{
		memset(this, 0, sizeof(MeshletDesc));
	}

	union
	{
		struct
		{
            unsigned bboxMinX : 8;
            unsigned bboxMinY : 8;
            unsigned bboxMinZ : 8;
            unsigned vertexMax : 8;

            unsigned bboxMaxX : 8;
            unsigned bboxMaxY : 8;
            unsigned bboxMaxZ : 8;
            unsigned primMax : 8;

            unsigned vertexBegin : 20;
            signed coneOctX : 8;
            unsigned coneAngleLo : 4;

            unsigned primBegin : 20;
            signed coneOctY : 8;
            unsigned coneAngleHi : 4;
		};

		struct
		{
			std::uint32_t m_x;
			std::uint32_t m_y;
			std::uint32_t m_z;
			std::uint32_t m_w;
		};
	};

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

	static std::uint32_t Pack(std::uint32_t value, int width, int offset)
	{
    	return (std::uint32_t)((value & ((1 << width) - 1)) << offset);
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

    static std::uint32_t Unpack(std::uint32_t value, int width, int offset)
	{
    	return (std::uint32_t)((value >> offset) & ((1 << width) - 1));
    }
};

class MeshletBuilder
{
	// todo
};
