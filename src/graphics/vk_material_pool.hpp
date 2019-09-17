/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "../material_pool.hpp"

#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace gfx
{

	class Context;
	class DescriptorHeap;

	class VkMaterialPool : public MaterialPool
	{
	public:
		explicit VkMaterialPool(Context* context);
		~VkMaterialPool() final;

		std::uint32_t GetDescriptorSetID(MaterialHandle handle);
		gfx::DescriptorHeap* GetDescriptorHeap();

	private:
		void Load_Impl(MaterialHandle& handle, MaterialData const & data, TexturePool* texture_pool) final;

		Context* m_context;
		VkDescriptorSetLayout m_material_set_layout;
		std::unordered_map<std::uint32_t, std::uint32_t> m_descriptor_sets;

		gfx::DescriptorHeap* m_desc_heap;
	};

} /* gfx */
