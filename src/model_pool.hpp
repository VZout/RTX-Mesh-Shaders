/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "resource_structs.hpp"

#include "util/log.hpp"

struct ModelHandle
{
	std::vector<std::uint32_t> m_mesh_ids;
};

namespace gfx
{
	class CommandList;
}

class ModelPool
{
public:
	ModelPool();
	virtual ~ModelPool() = default;

	template<typename V_T, typename I_T = std::uint32_t>
	ModelHandle Load(ModelData* data);

	virtual void Stage(gfx::CommandList* command_list, std::uint32_t frame_idx) = 0;
	virtual void PostStage() = 0;

protected:
	virtual void AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
			void* index_data, std::uint32_t num_indices, std::uint32_t index_stride) = 0;

	std::uint32_t m_num_allocations;

};

#define DEFINE_HAS_STRUCT(name, member) \
namespace internal { \
template <typename T, typename = int> \
struct Has##name : std::false_type { }; \
template <typename T> \
struct Has##name <T, decltype((void) T::member, 0)> : std::true_type { }; \
} /* internal */

DEFINE_HAS_STRUCT(Pos, m_pos)
DEFINE_HAS_STRUCT(UV, m_uv)
DEFINE_HAS_STRUCT(Normal, m_normal)

template<typename V_T, typename I_T>
ModelHandle ModelPool::Load(ModelData* data)
{
	ModelHandle model_handle;

	for (auto const & mesh : data->m_meshes)
	{
		auto num_vertices = mesh.m_positions.size();
		auto num_indices = mesh.m_indices.size();

		std::vector<V_T> vertices(num_vertices);
		std::vector<I_T> indices = mesh.m_indices;

		for (std::size_t i = 0; i < num_vertices; i++)
		{
			using namespace internal;
			if constexpr (HasPos<V_T>::value) { vertices[i].m_pos = mesh.m_positions[i]; }
			if constexpr (HasUV<V_T>::value) { vertices[i].m_uv = {mesh.m_uvw[i].x, mesh.m_uvw[i].y }; }
			if constexpr (HasNormal<V_T>::value) { vertices[i].m_normal = mesh.m_normals[i]; }
		}

		AllocateMesh(vertices.data(), num_vertices, sizeof(V_T), indices.data(), num_indices, sizeof(I_T));
		model_handle.m_mesh_ids.emplace_back(m_num_allocations);
		m_num_allocations++;
	}

	return model_handle;
}

#undef DEFINE_HAS_STRUCT