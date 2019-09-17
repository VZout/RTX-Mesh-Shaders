/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "material_pool.hpp"

#include "texture_pool.hpp"
#include "graphics/descriptor_heap.hpp"

MaterialPool::MaterialPool()
	: m_next_id(0)
{
}


MaterialHandle MaterialPool::Load(MaterialData const & data, TexturePool* texture_pool)
{
	auto new_id = m_next_id;

	MaterialHandle handle;
	handle.m_material_id = new_id;
	handle.m_albedo_texture_handle = texture_pool->Load(data.m_albedo_texture);
	handle.m_normal_texture_handle = texture_pool->Load(data.m_normal_map_texture);

	Load_Impl(handle, data, texture_pool);

	m_next_id++;

	return handle;
}