#pragma once

#include <cstdint>
#include <cassert>
#include <cstring>

static inline const int max_vertex_count_limit = 256;
static inline const int primitive_packing_alignment = 32;
static inline const int max_primitive_count_limit = 256;
static inline const std::uint32_t vertex_packing_alignment = 16;
static inline const std::uint32_t meshlets_per_task = 32;

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
		//return Unpack(m_w, 20, 0) * primitive_packing_alignment;
		return Unpack(m_w, 20, 0);
	}

	void SetPrimBegin(std::uint32_t begin) {
		//assert(begin % primitive_packing_alignment == 0);
		//assert(begin / primitive_packing_alignment < ((1 << 20) - 1));
		//m_w |= Pack(begin / primitive_packing_alignment, 20, 0);
		m_w |= Pack(begin, 20, 0);
	}

	std::uint32_t GetVertexBegin() const
	{ 
		return Unpack(m_z, 20, 0) * vertex_packing_alignment;
	}

	void SetVertexBegin(std::uint32_t begin)
	{
		//assert(begin % vertex_packing_alignment == 0);
		//assert(begin / vertex_packing_alignment < ((1 << 20) - 1));
		//m_z |= Pack(begin / vertex_packing_alignment, 20, 0
		m_z |= Pack(begin, 20, 0);
	}

	static std::uint32_t Pack(std::uint32_t value, int width, int offset)
	{
    	return (std::uint32_t)((value & ((1 << width) - 1)) << offset);
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
