/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>
#include <vec3.hpp>
#include <vec4.hpp>
#include <optional>

struct TextureData
{
	std::uint32_t m_width;
	std::uint32_t m_height;
	std::uint32_t m_channels;
	std::vector<unsigned char> m_pixels;
};

struct MaterialData
{
	TextureData m_albedo_texture;
	TextureData m_metallic_texture;
	TextureData m_roughness_texture;
	TextureData m_ambient_occlusion_texture;
	TextureData m_normal_map_texture;
	TextureData m_emissive_texture;

	float m_base_color[3];
	float m_base_metallic;
	float m_base_roughness;
	float m_base_transparency;
	float m_base_emissive;
};

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
	std::vector<MaterialData> m_materials;
};

struct RenderTargetProperties
{
	/*using IsRenderWindow = util::NamedType<bool>;
	using Width = util::NamedType<std::optional<unsigned int>>;
	using Height = util::NamedType<std::optional<unsigned int>>;
	using ExecuteResourceState = util::NamedType<std::optional<ResourceState>>;
	using FinishedResourceState = util::NamedType<std::optional<wr::ResourceState>>;
	using CreateDSVBuffer = util::NamedType<bool>;
	using DSVFormat = util::NamedType<Format>;
	using RTVFormats = util::NamedType<std::array<Format, 8>>;
	using NumRTVFormats = util::NamedType<unsigned int>;
	using ResolutionScalar = util::NamedType<float>;*/

	bool m_is_render_window;
	std::optional<std::uint32_t> m_width;
	std::optional<std::uint32_t> m_height;
	//ExecuteResourceState m_state_execute;
	//FinishedResourceState m_state_finished;
	//CreateDSVBuffer m_create_dsv_buffer;
	//DSVFormat m_dsv_format;
	//RTVFormats m_rtv_formats;
	//NumRTVFormats m_num_rtv_formats;

	bool m_clear = false;
	bool m_clear_depth = false;

	float m_resolution_scale = 1;
};