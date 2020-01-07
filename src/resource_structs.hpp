/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>
#include <vec2.hpp>
#include <vec3.hpp>
#include <optional>
#include <vulkan/vulkan.h>

struct TextureData
{
	std::uint32_t m_width = -1;
	std::uint32_t m_height = -1;
	std::uint32_t m_channels = -1;
	bool m_is_hdr = false;
	void* m_pixels = nullptr;
};

struct MaterialData
{
	TextureData m_albedo_texture;
	TextureData m_metallic_texture;
	TextureData m_roughness_texture;
	TextureData m_ambient_occlusion_texture;
	TextureData m_normal_map_texture;
	TextureData m_emissive_texture;
	TextureData m_thickness_texture;
	TextureData m_displacement_texture;

	glm::vec3 m_base_color = { -1, -1, -1 };
	float m_base_metallic = -1;
	float m_base_roughness = -1;
	float m_base_reflectivity = 0.5f;
	float m_base_transparency = -1;
	float m_base_emissive = -1;
	float m_base_normal_strength = 1;
	float m_base_anisotropy = 0;
	glm::vec2 m_base_anisotropy_dir = { 1, 0 };
	float m_base_clear_coat = 0;
	float m_base_clear_coat_roughness = 0;
	glm::vec2 m_base_uv_scale = { 1, 1 };
	bool m_two_sided = true;
};

struct ExtraMaterialData
{
	std::vector<const char*> m_thickness_texture_paths;
	std::vector<const char*> m_displacement_texture_paths;
};

struct MeshData
{
	std::vector<glm::vec3> m_positions;
	std::vector<glm::vec3> m_normals;
	std::vector<glm::vec3> m_uvw;
	std::vector<glm::vec3> m_tangents;
	std::vector<glm::vec3> m_bitangents;

	std::vector<unsigned char> m_indices;
	std::size_t m_num_indices;
	std::size_t m_indices_stride;

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
	//CreateDSVBuffer m_create_dsv_buffer;
	VkFormat m_dsv_format;
	std::vector<VkFormat> m_rtv_formats;
	std::optional<VkImageLayout> m_state_execute;
	std::optional<VkImageLayout> m_state_finished;

	bool m_clear = false;
	bool m_clear_depth = false;
	bool m_allow_direct_access = false;
	bool m_is_cube_map = false;
	std::uint32_t m_mip_levels = 1;

	float m_resolution_scale = 1;

	bool m_bind_by_default = true;
};