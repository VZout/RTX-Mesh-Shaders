/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <string>
#include <vector>
#include <vec3.hpp>
#include <vec4.hpp>

struct MeshData
{
	std::vector<glm::vec3> m_positions;
	std::vector<glm::vec3> m_colors;
	std::vector<glm::vec3> m_normals;
	std::vector<glm::vec3> m_uvw;
	std::vector<glm::vec3> m_tangents;
	std::vector<glm::vec3> m_bitangents;

	std::vector<glm::vec4> m_bone_weights;
	std::vector<glm::vec4> m_bone_ids;

	std::vector<std::uint32_t> m_indices;

	std::uint32_t m_material_id;
};

struct ModelData
{
	std::vector<MeshData> m_meshes;
};

class ModelLoader
{
public:
	explicit ModelLoader(std::vector<std::string> const & supported_formats);
	virtual ~ModelLoader();

	ModelData* Load(std::string const & path);
	void Remove(ModelData* model_data);

protected:
	virtual ModelData* LoadFromDisc(std::string const & path) = 0;

	std::vector<std::string> m_supported_formats;
	std::vector<ModelData*> m_loaded_models;
};
