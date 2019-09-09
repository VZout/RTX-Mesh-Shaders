/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace gfx
{

	class Context;
	class RootSignature;
	class GPUBuffer;

	class DescHeapHandle
	{
		VkDescriptorSet m_descriptor_set;
	};

	class DescriptorHeap
	{
		friend class CommandList;
	public:
		explicit DescriptorHeap(Context* context, RootSignature* root_signature);
		~DescriptorHeap();

		void CreateSRVFromCB(GPUBuffer* buffer, std::uint32_t handle);

	private:
		VkDescriptorPoolCreateInfo m_descriptor_pool_create_info;
		VkDescriptorPool m_descriptor_pool;
		std::vector<VkDescriptorSet> m_descriptor_sets;

		Context* m_context;
	};

} /* gfx */