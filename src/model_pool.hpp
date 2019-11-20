/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <algorithm>
#include <unordered_map>

#include "resource_loader.hpp"
#include "resource_structs.hpp"
#include "material_pool.hpp"
#include "texture_pool.hpp"
#include "meshlet_builder.hpp"
#include "stb_image_loader.hpp"

#include "util/log.hpp"

struct ModelHandle
{
	struct MeshHandle
	{
		std::uint32_t m_id;
		std::uint32_t m_num_indices;
		std::uint32_t m_num_vertices;
		std::optional<MaterialHandle> m_material_handle;

		bool operator==(MeshHandle const & other) const
		{
			return m_id == other.m_id &&
				m_num_indices == other.m_num_indices &&
				m_num_vertices == other.m_num_vertices &&
				m_material_handle == other.m_material_handle;
		}
	};

	std::vector<MeshHandle> m_mesh_handles;

	bool operator==(ModelHandle const & other) const
	{
		return m_mesh_handles == other.m_mesh_handles;
	}
};

namespace std {

	template <>
	struct hash<ModelHandle>
	{
		std::size_t operator()(const ModelHandle& handle) const
		{
			std::size_t sum = 0;
			for (auto const & v : handle.m_mesh_handles)
			{
				sum += (std::hash<std::uint32_t>()(v.m_id)
						^ (std::hash<std::uint32_t>()(v.m_num_indices) << 1) >> 1)
						^ (std::hash<std::uint32_t>()(v.m_num_vertices) << 1);
			}

			return sum;
		}
	};

}

namespace gfx
{
	class CommandList;
}

class ModelPool
{
public:
	ModelPool();
	virtual ~ModelPool() = default;

	template<typename V_T>
	ModelHandle Load(std::string const & path,
		bool store_data = false);
	template<typename V_T>
	ModelHandle LoadWithMaterials(std::string const & path,
		MaterialPool* material_pool,
		TexturePool* texture_pool,
		bool store_data = false,
		std::optional<ExtraMaterialData> extra = std::nullopt);
	template<typename V_T>
	ModelHandle Load(ModelData* data);
	template<typename V_T>
	ModelHandle LoadWithMaterials(ModelData* data,
		MaterialPool* material_pool,
		TexturePool* texture_pool,
		std::optional<ExtraMaterialData> extra = std::nullopt);
	ModelData* GetRawData(ModelHandle handle);

	virtual void Stage(gfx::CommandList* command_list) = 0;
	virtual void PostStage() = 0;

	template<typename T>
	static void RegisterLoader();

	std::unordered_map<ModelHandle, ModelData*> m_loaded_data; // TODO: Make private
protected:
	virtual void AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
			void* index_data, std::uint32_t num_indices, std::uint32_t index_stride, void* meshlet_data, std::uint32_t num_meshlets) = 0;

	std::uint32_t m_next_id;

	inline static std::vector<ResourceLoader<ModelData>*> m_registered_loaders = {};
};

template<typename T>
void ModelPool::RegisterLoader()
{
	m_registered_loaders.push_back(new T());
}

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
DEFINE_HAS_STRUCT(Tangent, m_tangent)
DEFINE_HAS_STRUCT(Bitangent, m_bitangent)

template<typename V_T>
ModelHandle ModelPool::Load(std::string const & path,
	bool store_data)
{
	return LoadWithMaterials<V_T>(path, nullptr, nullptr, store_data);
}

template<typename V_T>
ModelHandle ModelPool::LoadWithMaterials(std::string const & path,
	MaterialPool* material_pool,
	TexturePool* texture_pool,
	bool store_data,
	std::optional<ExtraMaterialData> extra)
{
	auto extension = path.substr(path.find_last_of('.') + 1);

	for (auto& loader : m_registered_loaders)
	{
		if (loader->IsSupportedExtension(extension))
		{
			auto model_data = loader->Load(path);

			auto handle = LoadWithMaterials<V_T>(model_data, material_pool, texture_pool, extra);

			if (store_data)
			{
				m_loaded_data.insert({ handle, model_data });
			}
			else
			{
				delete model_data;
			}

			return handle;
		}
	}

	LOGE("Could not find a appropriate model loader.");

	return ModelHandle{};
}

template<typename V_T>
ModelHandle ModelPool::Load(ModelData* data)
{
	return LoadWithMaterials<V_T>(data, nullptr, nullptr);
}

template<typename V_T>
ModelHandle ModelPool::LoadWithMaterials(ModelData* data,
	MaterialPool* material_pool,
	TexturePool* texture_pool,
	std::optional<ExtraMaterialData> extra)
{
	ModelHandle model_handle;

	bool mat_and_texture_pool_available = material_pool && texture_pool;

	// Apply extra material data
	if (extra.has_value())
	{
		auto image_loader = new STBImageLoader(); // TODO: Memory leak

		const auto& thickness_paths = extra.value().m_thickness_texture_paths;
		for (std::size_t i = 0; i < std::min(data->m_materials.size(), thickness_paths.size()); i++)
		{
			data->m_materials[i].m_thickness_texture = *image_loader->LoadFromDisc(thickness_paths[i]).get();
		}

		const auto& displacement_paths = extra.value().m_displacement_texture_paths;
		for (std::size_t i = 0; i < std::min(data->m_materials.size(), displacement_paths.size()); i++)
		{
			data->m_materials[i].m_displacement_texture = *image_loader->LoadFromDisc(displacement_paths[i]).get();
		}
	}

	std::unordered_map<std::uint32_t, MaterialHandle> loaded_materials;

	for (auto const & mesh : data->m_meshes)
	{
		std::optional<MaterialHandle> material_handle = std::nullopt;

		if (mat_and_texture_pool_available)
		{
			// If we already loaded that material use that one
			if (auto it = loaded_materials.find(mesh.m_material_id); it != loaded_materials.end())
			{
				material_handle = it->second;
			}
			else // if we haven't loaded the material load it.
			{
				material_handle = material_pool->Load(data->m_materials[mesh.m_material_id], texture_pool);
				loaded_materials.insert({ mesh.m_material_id, material_handle.value() });
			}
		}

		auto num_vertices = mesh.m_positions.size();
		auto num_indices = mesh.m_num_indices;
		auto index_stide = mesh.m_indices_stride;

		std::vector<V_T> vertices(num_vertices);
		auto indices = mesh.m_indices;

		for (std::size_t i = 0; i < num_vertices; i++)
		{
			using namespace internal;
			if constexpr (HasPos<V_T>::value) { vertices[i].m_pos = mesh.m_positions[i]; }
			if constexpr (HasUV<V_T>::value) { vertices[i].m_uv = {mesh.m_uvw[i].x, mesh.m_uvw[i].y }; }
			if constexpr (HasNormal<V_T>::value) { vertices[i].m_normal = mesh.m_normals[i]; }
			if constexpr (HasTangent<V_T>::value) { vertices[i].m_tangent = mesh.m_tangents[i]; }
			if constexpr (HasBitangent<V_T>::value) { vertices[i].m_bitangent = mesh.m_bitangents[i]; }
		}

		// Generate meshlets
		std::vector<MeshletDesc> meshlet_data;
		int max_meshlet_indices = 63;

		for (auto indices_start = 0; indices_start < mesh.m_num_indices; indices_start += max_meshlet_indices)
		{
			MeshletDesc meshlet = {};

			auto num_indices_in_meshlet = std::min((int)mesh.m_num_indices - indices_start, max_meshlet_indices);
			std::vector<int> used_vertex_indices;

			for (auto i = indices_start; i < indices_start + num_indices_in_meshlet; i += 3)
			{
				glm::ivec3 triangle;
				memcpy(&triangle, &mesh.m_indices[i * mesh.m_indices_stride], mesh.m_indices_stride * 3);

				for (auto k = 0; k < 3; k++)
				{
					if (std::find(used_vertex_indices.begin(), used_vertex_indices.end(), triangle[k]) == used_vertex_indices.end())
					{
						used_vertex_indices.push_back(triangle[k]);
					}
				}
			}

			meshlet.SetNumVertices(24);
			meshlet.SetNumPrims(num_indices_in_meshlet / 3);
			meshlet.SetPrimBegin(indices_start / 3);

			meshlet_data.push_back(meshlet);
		}

		AllocateMesh(vertices.data(), num_vertices, sizeof(V_T), indices.data(), num_indices, index_stide, meshlet_data.data(), meshlet_data.size());
		model_handle.m_mesh_handles.emplace_back(ModelHandle::MeshHandle{
			.m_id = m_next_id,
			.m_num_indices = static_cast<std::uint32_t>(num_indices),
			.m_num_vertices = static_cast<std::uint32_t>(num_vertices),
			.m_material_handle = material_handle
		});
		m_next_id++;
	}

	return model_handle;
}

#undef DEFINE_HAS_STRUCT