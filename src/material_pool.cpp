/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "material_pool.hpp"

#include "texture_pool.hpp"
#include "util/log.hpp"

MaterialPool::MaterialPool()
	: m_loaded_defaults(false), m_default_albedo_texture(0), m_default_roughness_metallic_texture(0), m_default_normal_texture(0), m_next_id(0)
{

}

MaterialHandle MaterialPool::Load(MaterialData const & data, TexturePool* texture_pool)
{
	auto new_id = m_next_id;

	if (!m_loaded_defaults)
	{
		m_loaded_defaults = true;
		m_default_albedo_texture = texture_pool->Load("white.png", false);
		m_default_normal_texture = texture_pool->Load("flat_normal.png", false);
		m_default_roughness_metallic_texture = texture_pool->Load("rough1metal0ao1.png", false);
		m_default_thickness_texture = texture_pool->Load("white.png", false);
		m_default_displacement_texture = texture_pool->Load("black.png", false);
	}

	MaterialHandle handle;
	handle.m_material_id = new_id;
	handle.m_albedo_texture_handle = data.m_albedo_texture.m_pixels ? texture_pool->Load(data.m_albedo_texture, true, true) : m_default_albedo_texture;
	handle.m_normal_texture_handle = data.m_normal_map_texture.m_pixels ? texture_pool->Load(data.m_normal_map_texture, true) : m_default_normal_texture;
	handle.m_roughness_texture_handle = data.m_roughness_texture.m_pixels ? texture_pool->Load(data.m_roughness_texture, true) : m_default_roughness_metallic_texture;
	handle.m_thickness_texture_handle = data.m_thickness_texture.m_pixels ? texture_pool->Load(data.m_thickness_texture, true) : m_default_thickness_texture;
	handle.m_displacement_texture_handle = data.m_displacement_texture.m_pixels ? texture_pool->Load(data.m_displacement_texture, true) : m_default_displacement_texture;

	m_raw_data[new_id] = data;

	Load_Impl(handle, data, texture_pool);

	m_next_id++;

	return handle;
}

MaterialData MaterialPool::GetRawData(MaterialHandle handle)
{
	if (auto it = m_raw_data.find(handle.m_material_id); it != m_raw_data.end())
	{
		return it->second;
	}

	LOGE("Failed to get raw material data from material handle");
	return MaterialData();
}
