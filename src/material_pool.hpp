/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>

#include "resource_loader.hpp"
#include "resource_structs.hpp"

class TexturePool;

struct MaterialHandle
{
	std::uint32_t m_material_id;
	std::uint32_t m_albedo_texture_handle;
	std::uint32_t m_normal_texture_handle;
	std::uint32_t m_material_set_id;


	bool operator==(MaterialHandle const & other) const
	{
		return m_material_id == other.m_material_id;
	}
};

class MaterialPool
{
public:
	MaterialPool();
	virtual ~MaterialPool() = default;

	MaterialHandle Load(MaterialData const & data, TexturePool* texture_pool);

private:
	virtual void Load_Impl(MaterialHandle& handle, MaterialData const & data, TexturePool* texture_pool) = 0;

	std::uint32_t m_next_id;
};