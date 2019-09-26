/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "../constant_buffer_pool.hpp"

namespace gfx
{

	class Context;
	class DescriptorHeap;
	class GPUBuffer;

	class VkConstantBufferPool : public ConstantBufferPool
	{
	public:
		explicit VkConstantBufferPool(Context* context, std::uint32_t binding, VkShaderStageFlags flags = VK_SHADER_STAGE_VERTEX_BIT);
		~VkConstantBufferPool() final;

		void Update(ConstantBufferHandle handle, std::uint64_t size, void* data, std::uint32_t frame_idx, std::uint64_t offset = 0) final;

		gfx::DescriptorHeap* GetDescriptorHeap();

	private:
		void Allocate_Impl(ConstantBufferHandle& handle, std::uint64_t size) final;

		Context* m_context;

		std::uint32_t m_binding;
		VkDescriptorSetLayout m_cb_set_layout;

		gfx::DescriptorHeap* m_desc_heap;
		std::vector<std::vector<GPUBuffer*>> m_buffers;

	};

} /* gfx */