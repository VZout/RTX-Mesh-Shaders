/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

class Renderer;

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

		friend class ::Renderer; // TODO: remove me bich
	public:
		struct Desc
		{
			std::uint32_t m_num_descriptors = 1;
			std::uint32_t m_versions = 1;
		};

		explicit DescriptorHeap(Context* context, RootSignature* root_signature, Desc desc);
		~DescriptorHeap();

		void CreateSRVFromCB(GPUBuffer* buffer, std::uint32_t handle, std::uint32_t frame_idx);

	private:
		Context* m_context;

		Desc m_desc;
		VkDescriptorPoolCreateInfo m_descriptor_pool_create_info;
		std::vector<VkDescriptorPool> m_descriptor_pools;
		std::vector<std::vector<VkDescriptorSet>> m_descriptor_sets; // first array is versions second array is sets

	};

} /* gfx */