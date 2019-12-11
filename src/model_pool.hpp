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
#include <glm.hpp>

#include "util/log.hpp"

struct ModelHandle
{
	struct MeshOffsets
	{
		std::uint64_t m_vb;
		std::uint64_t m_ib;
	};

	struct MeshHandle
	{
		std::uint32_t m_id;
		MeshOffsets m_offsets;
		std::uint32_t m_num_indices;
		std::uint32_t m_num_vertices;
		std::uint64_t m_vertex_stride;
		std::uint64_t m_index_stride;
		std::optional<MaterialHandle> m_material_handle;

		glm::vec3 m_bbox_min;
		glm::vec3 m_bbox_max;

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
	virtual ModelHandle::MeshOffsets AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
			void* index_data, std::uint32_t num_indices, std::uint32_t index_stride, void* meshlet_data, std::uint32_t num_meshlets) = 0;

	virtual void AllocateMeshShadingBuffers(std::vector<std::uint32_t> vertex_indices, std::vector<std::uint8_t> flat_indices) = 0;

	std::uint32_t m_next_id;

	inline static std::vector<ResourceLoader<ModelData>*> m_registered_loaders = {};
};

inline glm::vec4 GetBoxCorner(glm::vec3 bboxMin, glm::vec3 bboxMax, int n)
{
	using glm::vec4;
	switch (n) {
	case 0:
		return vec4(bboxMin.x, bboxMin.y, bboxMin.z, 1);
	case 1:
		return vec4(bboxMax.x, bboxMin.y, bboxMin.z, 1);
	case 2:
		return vec4(bboxMin.x, bboxMax.y, bboxMin.z, 1);
	case 3:
		return vec4(bboxMax.x, bboxMax.y, bboxMin.z, 1);
	case 4:
		return vec4(bboxMin.x, bboxMin.y, bboxMax.z, 1);
	case 5:
		return vec4(bboxMax.x, bboxMin.y, bboxMax.z, 1);
	case 6:
		return vec4(bboxMin.x, bboxMax.y, bboxMax.z, 1);
	case 7:
		return vec4(bboxMax.x, bboxMax.y, bboxMax.z, 1);
	}
}

// all oct functions derived from "A Survey of Efficient Representations for Independent Unit Vectors"
// http://jcgt.org/published/0003/02/01/paper.pdf
inline glm::vec3 OctSignNotZero(glm::vec3 v)
{
	// leaves z as is
	return glm::vec3((v.x >= 0.0f) ? +1.0f : -1.0f, (v.y >= 0.0f) ? +1.0f : -1.0f, 1.0f);
}

inline glm::vec3 OctToFVec3(glm::vec3 e)
{
	auto v = glm::vec3(e.x, e.y, 1.0f - fabsf(e.x) - fabsf(e.y));
	if (v.z < 0.0f)
	{
		v = glm::vec3(1.0f - fabs(v.y), 1.0f - fabs(v.x), v.z) * OctSignNotZero(v);
	}
	return glm::normalize(v);
}

inline glm::vec3 FVec3ToOct(glm::vec3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
	glm::vec3 p = glm::vec3(v.x, v.y, 0) * (1.0f / (fabsf(v.x) + fabsf(v.y) + fabsf(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return (v.z <= 0.0f) ? glm::vec3(1.0f - fabsf(p.y), 1.0f - fabsf(p.x), 0.0f) * OctSignNotZero(p) : p;
}

inline glm::vec3 FVec3ToOctnPrecise(glm::vec3 v, const int n)
{
	glm::vec3 s = FVec3ToOct(v);  // Remap to the square
								  // Each snorm's max value interpreted as an integer,
								  // e.g., 127.0 for snorm8
	float M = float(1 << ((n / 2) - 1)) - 1.0;
	// Remap components to snorm(n/2) precision...with floor instead
	// of round (see equation 1)
	s = glm::floor(glm::clamp(s, glm::vec3(-1.0f), glm::vec3(1.0f)) * M) * glm::vec3(1.0 / M);
	glm::vec3 bestRepresentation = s;
	float highestCosine = glm::dot(OctToFVec3(s), v);
	// Test all combinations of floor and ceil and keep the best.
	// Note that at +/- 1, this will exit the square... but that
	// will be a worse encoding and never win.
	for (int i = 0; i <= 1; ++i)
		for (int j = 0; j <= 1; ++j)
			// This branch will be evaluated at compile time
			if ((i != 0) || (j != 0))
			{
				// Offset the bit pattern (which is stored in floating
				// point!) to effectively change the rounding mode
				// (when i or j is 0: floor, when it is one: ceiling)
				glm::vec3 candidate = glm::vec3(i, j, 0) * (1 / M) + s;
				float cosine = glm::dot(OctToFVec3(candidate), v);
				if (cosine > highestCosine)
				{
					bestRepresentation = candidate;
					highestCosine = cosine;
				}
			}
	return bestRepresentation;
}

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

		glm::vec3 average_normal(0);
		std::vector<glm::vec3> tri_normals;

		// Calculate objectbbox
		glm::vec3 object_bbox_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 object_bbox_max = glm::vec3(-std::numeric_limits<float>::max());
		for (auto i = 0; i < mesh.m_num_indices; i += 3)
		{
			glm::ivec3 triangle;
			memcpy(&triangle, &mesh.m_indices[i * mesh.m_indices_stride], mesh.m_indices_stride * 3);

			// bounding box
			{
				object_bbox_min = glm::min(object_bbox_min, vertices[triangle[0]].m_pos);
				object_bbox_min = glm::min(object_bbox_min, vertices[triangle[1]].m_pos);
				object_bbox_min = glm::min(object_bbox_min, vertices[triangle[2]].m_pos);

				object_bbox_max = glm::max(object_bbox_max, vertices[triangle[0]].m_pos);
				object_bbox_max = glm::max(object_bbox_max, vertices[triangle[1]].m_pos);
				object_bbox_max = glm::max(object_bbox_max, vertices[triangle[2]].m_pos);
			}

			// cone
			{
				glm::vec3 cross = glm::cross(vertices[triangle[1]].m_pos - vertices[triangle[0]].m_pos, vertices[triangle[2]].m_pos - vertices[triangle[0]].m_pos);
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

				average_normal += normal;
				tri_normals.push_back(normal);
			}
		}

		// Generate meshlets
		std::vector<MeshletDesc> meshlet_data;
		std::vector<std::uint32_t> vertex_indices; // used to index the vertex buffer from mesh shading (Uploaded to the GPU)
		std::vector<std::uint8_t> index_indices; // used to index the vertex indices buffer  (Uploaded to the GPU)
		int max_meshlet_indices = 63;

		int vertices_start = 0;
		int prim_begin = 0;

		for (auto indices_start = 0; indices_start < mesh.m_num_indices; indices_start += max_meshlet_indices)
		{
			MeshletDesc meshlet = {};

			glm::vec3 bbox_min = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 bbox_max = glm::vec3(-std::numeric_limits<float>::max());

			auto num_indices_in_meshlet = std::min((int)mesh.m_num_indices - indices_start, max_meshlet_indices);
			std::vector<std::uint32_t> meshlet_vertex_indices;
			std::vector<std::uint32_t> meshlet_indices;

			// obtain all indices required for this meshlet.
			for (auto i = indices_start; i < indices_start + num_indices_in_meshlet; i += 3)
			{
				glm::ivec3 triangle;
				memcpy(&triangle, &mesh.m_indices[i * mesh.m_indices_stride], mesh.m_indices_stride * 3);

				// bounding box
				{
					bbox_min = glm::min(bbox_min, vertices[triangle[0]].m_pos);
					bbox_min = glm::min(bbox_min, vertices[triangle[1]].m_pos);
					bbox_min = glm::min(bbox_min, vertices[triangle[2]].m_pos);

					bbox_max = glm::max(bbox_max, vertices[triangle[0]].m_pos);
					bbox_max = glm::max(bbox_max, vertices[triangle[1]].m_pos);
					bbox_max = glm::max(bbox_max, vertices[triangle[2]].m_pos);
				}

				for (auto k = 0; k < 3; k++)
				{
					meshlet_indices.push_back(triangle[k]);
				}
			}

			// Create the vertex indices vector based on the flat indices vector.
			std::vector<std::uint8_t> flat_meshlet_indices(meshlet_indices.size());
			std::vector<std::pair<std::uint32_t, std::uint8_t>> flat_helper; // first = original, second = flat

			for (auto i = 0; i < meshlet_indices.size(); i++)
			{
				auto index = meshlet_indices[i];

				if (std::find(meshlet_vertex_indices.begin(), meshlet_vertex_indices.end(), index) == meshlet_vertex_indices.end())
				{
					meshlet_vertex_indices.push_back(index);
					auto new_flat_idx = meshlet_vertex_indices.size() - 1;
					flat_meshlet_indices[i] = new_flat_idx;
					flat_helper.push_back({ index, new_flat_idx });
				}
				else
				{
					bool found_match = false;
					for (auto const & pair : flat_helper)
					{
						if (pair.first == index)
						{
							flat_meshlet_indices[i] = pair.second;
							found_match = true;
							break;
						}
					}

					if (!found_match)
					{
						LOGW("The flattening code appears to be broken");
					}
				}
			}

			// alignment
			auto alligned_vertices_start = SizeAlignTwoPower(vertices_start, vertex_packing_alignment);
			auto alligned_prim_start = SizeAlignTwoPower(prim_begin, primitive_packing_alignment);

			// pad array to allignment
			for (auto i = 0; i < alligned_vertices_start - vertices_start; i++)
			{
				vertex_indices.push_back(0);
			}
			for (auto i = 0; i < alligned_prim_start - prim_begin; i++)
			{
				flat_meshlet_indices.push_back(0);
				flat_meshlet_indices.push_back(0);
				flat_meshlet_indices.push_back(0);
			}

			// Get new size with padding.
			int num_unique_vertices = meshlet_vertex_indices.size();
			int num_unique_indices = flat_meshlet_indices.size();

			//flat_meshlet_indices.push_back(69);

			vertices_start = alligned_vertices_start;
			prim_begin = alligned_prim_start;

			meshlet.SetNumVertices(num_unique_vertices);
			meshlet.SetVertexBegin(vertices_start);
			meshlet.SetNumPrims((num_indices_in_meshlet / 3));
			meshlet.SetPrimBegin(prim_begin);

			vertices_start += num_unique_vertices;
			prim_begin += (num_unique_indices / 3);

			vertex_indices.insert(vertex_indices.end(), meshlet_vertex_indices.begin(), meshlet_vertex_indices.end());
			index_indices.insert(index_indices.end(), flat_meshlet_indices.begin(), flat_meshlet_indices.end());

			// Do bounding box
			// truncate min relative to object min
			glm::vec3 object_bbox_extent = object_bbox_max - object_bbox_min;
			bbox_min = bbox_min - object_bbox_min;
			bbox_max = bbox_max - object_bbox_min;
			bbox_min = bbox_min / object_bbox_extent;
			bbox_max = bbox_max / object_bbox_extent;

			// Snap to grid
			const int grid_bits = 8;
			const int grid_last = (1 << grid_bits) - 1;
			uint8_t   grid_min[3];
			uint8_t   grid_max[3];

			grid_min[0] = std::max(0, std::min(int(truncf(bbox_min.x * float(grid_last))), grid_last - 1));
			grid_min[1] = std::max(0, std::min(int(truncf(bbox_min.y * float(grid_last))), grid_last - 1));
			grid_min[2] = std::max(0, std::min(int(truncf(bbox_min.z * float(grid_last))), grid_last - 1));
			grid_max[0] = std::max(0, std::min(int(ceilf(bbox_max.x * float(grid_last))), grid_last));
			grid_max[1] = std::max(0, std::min(int(ceilf(bbox_max.y * float(grid_last))), grid_last));
			grid_max[2] = std::max(0, std::min(int(ceilf(bbox_max.z * float(grid_last))), grid_last));

			meshlet.SetBBox(grid_min, grid_max);

			// potential improvement, instead of average maybe use
			// http://www.cs.technion.ac.il/~cggc/files/gallery-pdfs/Barequet-1.pdf
			float len = glm::length(average_normal);
			if (len > FLT_EPSILON)
			{
				average_normal = average_normal / len;
			}
			else
			{
				average_normal = glm::vec3(0.0f);
			}

			glm::vec3 packed = FVec3ToOctnPrecise(average_normal, 16);
			std::int8_t cone_x = std::min(127, std::max(-127, std::int32_t(packed.x * 127.0f)));
			std::int8_t cone_y = std::min(127, std::max(-127, std::int32_t(packed.y * 127.0f)));

			// post quantization normal
			average_normal = OctToFVec3(glm::vec3(float(cone_x) / 127.0f, float(cone_y) / 127.0f, 0.0f));

			float mindot = 1.0f;
			for (auto const & n : tri_normals)
			{
				mindot = std::min(mindot, glm::dot(n, average_normal));
			}

			// apply safety delta due to quantization
			mindot -= 1.0f / 127.0f;
			mindot = std::max(-1.0f, mindot);

			// positive value for cluster not being backface cullable (normals > 90)
			std::int8_t cone_angle = 127;
			if (mindot > 0)
			{
				// otherwise store -sin(cone angle)
				// we test against dot product (cosine) so this is equivalent to cos(cone angle + 90°)
				float angle = -sinf(acosf(mindot));
				cone_angle = std::max(-127, std::min(127, int32_t(angle * 127.0f)));
			}

			meshlet.SetCone(cone_x, cone_y, cone_angle);

			meshlet_data.push_back(meshlet);
		}

#ifdef DEBUG_SOMETHING
#define NVMSH_INDEX_BITS      8
#define NVMSH_PACKED4X8_GET(packed, idx)   (((packed) >> (NVMSH_INDEX_BITS * (idx))) & 255)

		std::vector<std::vector<std::uint32_t>> m_debug_write_indices_0(meshlet_data.size(), std::vector<std::uint32_t>(64));
		std::vector<std::vector<std::uint32_t>> m_debug_write_indices_1(meshlet_data.size(), std::vector<std::uint32_t>(64));
		std::vector<std::uint32_t> m_debug_indices;
		for (auto i = 0; i < index_indices.size(); i += 4)
		{
			uint32_t m_newint;
			memcpy(&m_newint, &index_indices[i], sizeof(std::uint32_t));

			UINT8 var1 = index_indices[i]; //0000 0001
			UINT8 var2 = 0;
			if (i + 1 < index_indices.size())
				var2 = index_indices[i+1]; //0000 0011
			UINT8 var3 = 0;
			if (i + 2 < index_indices.size())
				var3 = index_indices[i+2]; //0000 0111
			UINT8 var4 = 0;
			if (i + 3 < index_indices.size())
				var4 = index_indices[i+3]; //0000 1111
			UINT32 bigvar = (var1 << 24) + (var2 << 16) + (var3 << 8) + var4;

			m_debug_indices.push_back(bigvar);


			int q = 1;
		}

		int idx_x = 0;
		for (auto const & meshlet_desc : meshlet_data)
		{
			auto vert_max = meshlet_desc.GetNumVertices();
			auto prim_max = meshlet_desc.GetNumPrims();
			auto vert_begin = meshlet_desc.GetVertexBegin();
			auto prim_begin = meshlet_desc.GetPrimBegin();

			unsigned num_triangles = prim_max * 3;
			unsigned triangles_start = prim_begin * 3;
			for (auto i = triangles_start; i < triangles_start + num_triangles; i += 4)
			{

				std::uint32_t k = i - triangles_start;
				std::uint32_t ii = std::uint32_t(std::floor(float(i) / float(4)));
				if (ii >= m_debug_indices.size()) continue;
				std::uint32_t packed = m_debug_indices[ii];


				m_debug_write_indices_0[idx_x][(k)+0] = (NVMSH_PACKED4X8_GET((packed), 3 - 0));
				m_debug_write_indices_0[idx_x][(k)+1] = (NVMSH_PACKED4X8_GET((packed), 3 - 1));
				m_debug_write_indices_0[idx_x][(k)+2] = (NVMSH_PACKED4X8_GET((packed), 3 - 2));
				m_debug_write_indices_0[idx_x][(k)+3] = (NVMSH_PACKED4X8_GET((packed), 3 - 3));
			}
			idx_x++;
		}

		int idx_y = 0;
		for (auto const& meshlet_desc : meshlet_data)
		{
			auto vert_max = meshlet_desc.GetNumVertices();
			auto prim_max = meshlet_desc.GetNumPrims();
			auto vert_begin = meshlet_desc.GetVertexBegin();
			auto prim_begin = meshlet_desc.GetPrimBegin();

			unsigned num_indices = prim_max * 3;
			unsigned indices_start = prim_begin * 3;
			for (auto i = indices_start; i < indices_start + num_indices; i += 3)
			{
				std::uint32_t k = i - indices_start;
				auto q = indices_start + num_indices;

				if (i + 0 < index_indices.size())
					m_debug_write_indices_1[idx_y][k + 0] = index_indices[i + 0];
				if (i + 1 < index_indices.size())
					m_debug_write_indices_1[idx_y][k + 1] = index_indices[i + 1];
				if (i + 2 < index_indices.size())
					m_debug_write_indices_1[idx_y][k + 2] = index_indices[i + 2];
			}
			idx_y++;
		}

		for (auto k = 0; k < m_debug_write_indices_0.size(); k++)
		{
			for (auto i = 0; i < m_debug_write_indices_0[k].size(); i++)
			{
				//if (m_debug_write_indices_0[k][i] != m_debug_write_indices_1[k][i])
				{
					LOG("MESHIDX({}), IDX({}) optimized: {} default: {}", k, i, m_debug_write_indices_0[k][i], m_debug_write_indices_1[k][i]);
				}
			}
		}
#endif

#ifdef DEBUG_GENERATED_INDEX_BUFFERS
		for (auto meshlet : meshlet_data)
		{
			for (auto i = meshlet.GetPrimBegin() * 3; i < (meshlet.GetPrimBegin() + meshlet.GetNumPrims()) * 3; i += 3)
			{
				glm::ivec3 triangle;
				memcpy(&triangle, &mesh.m_indices[i * mesh.m_indices_stride], mesh.m_indices_stride * 3);

				glm::ivec3 triangle_new;
				auto vert_begin = meshlet.GetVertexBegin();
				triangle_new.x = vertex_indices[vert_begin + index_indices[i + 0]];
				triangle_new.y = vertex_indices[vert_begin + index_indices[i + 1]];
				triangle_new.z = vertex_indices[vert_begin + index_indices[i + 2]];		

				if (triangle_new != triangle)
				{
					LOG("=====================================");
					LOG("old school ({}, {}, {})", triangle.x, triangle.y, triangle.z);
					LOG("new school ({}, {}, {})", triangle_new.x, triangle_new.y, triangle_new.z);
				}
			}
		}
#endif

		AllocateMeshShadingBuffers(vertex_indices, index_indices);

		auto offsets = AllocateMesh(vertices.data(), num_vertices, sizeof(V_T), indices.data(), num_indices, index_stide, meshlet_data.data(), meshlet_data.size());

		model_handle.m_mesh_handles.emplace_back(ModelHandle::MeshHandle{
			.m_id = m_next_id,
			.m_offsets = offsets,
			.m_num_indices = static_cast<std::uint32_t>(num_indices),
			.m_num_vertices = static_cast<std::uint32_t>(num_vertices),
			.m_vertex_stride = sizeof(V_T),
			.m_index_stride = index_stide,
			.m_material_handle = material_handle,
			.m_bbox_min = object_bbox_min,
			.m_bbox_max = object_bbox_max
		});
		m_next_id++;
	}

	return model_handle;
}

#undef DEFINE_HAS_STRUCT