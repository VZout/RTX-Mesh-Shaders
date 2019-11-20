/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "vk_constant_buffer_pool.hpp"

#include "context.hpp"
#include "descriptor_heap.hpp"
#include "gfx_settings.hpp"
#include "gpu_buffers.hpp"
#include "../util/log.hpp"

gfx::VkConstantBufferPool::VkConstantBufferPool(Context* context, std::size_t buffer_size, std::size_t num_buffers, std::uint32_t binding, VkShaderStageFlags flags)
	: m_context(context), m_binding(binding), m_cb_set_layout(VK_NULL_HANDLE), m_desc_heap(nullptr)
{
	auto logical_device = context->m_logical_device;

	gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
	descriptor_heap_desc.m_versions = gfx::settings::num_back_buffers;
	descriptor_heap_desc.m_num_descriptors = num_buffers;
	m_desc_heap = new gfx::DescriptorHeap(m_context, descriptor_heap_desc);

	m_pool = new MemoryPool(m_context, buffer_size, num_buffers * gfx::settings::num_back_buffers);

	m_buffers.resize(gfx::settings::num_back_buffers);

	// TODO: make this entire layout static and use it when creating root signatures.
	std::vector<VkDescriptorSetLayoutBinding> parameters(1);
	parameters[0].binding = m_binding;
	parameters[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	parameters[0].descriptorCount = 1;
	parameters[0].stageFlags = flags;
	parameters[0].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptor_set_create_info = {};
	descriptor_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_create_info.bindingCount = parameters.size();
	descriptor_set_create_info.pBindings = parameters.data();

	if (vkCreateDescriptorSetLayout(logical_device, &descriptor_set_create_info, nullptr, &m_cb_set_layout) != VK_SUCCESS)
	{
		LOGC("failed to create descriptor set layout!");
	}
}

gfx::VkConstantBufferPool::~VkConstantBufferPool()
{
	auto logical_device = m_context->m_logical_device;

	for (auto & buffers_l : m_buffers)
	{
		for (auto & buffer : buffers_l)
		{
			buffer->Unmap();
			delete buffer;
		}
	}

	vkDestroyDescriptorSetLayout(logical_device, m_cb_set_layout, nullptr);

	delete m_desc_heap;
	delete m_pool;
}

void gfx::VkConstantBufferPool::Flush(std::uint32_t frame_idx)
{
	for (auto& buffer : m_buffers[frame_idx])
	{
		// Flush to make writes visible to GPU
		vmaFlushAllocation(m_context->m_vma_allocator, buffer->m_buffer_allocation, 0, VK_WHOLE_SIZE);
	}

}

std::vector<std::uint32_t> gfx::VkConstantBufferPool::CreateConstantBufferSet(std::vector<ConstantBufferHandle> handles)
{
	std::vector<std::uint32_t> retval;
	for (std::uint32_t frame_idx = 0; frame_idx < gfx::settings::num_back_buffers; frame_idx++)
	{
		std::vector<GPUBuffer*> buffers;
		for (auto const& handle : handles)
		{
			buffers.push_back(m_buffers[frame_idx][handle.m_cb_id]);
		}

		retval.emplace_back(m_desc_heap->CreateSRVSetFromCB(buffers, m_cb_set_layout, m_binding, frame_idx));
	}

	return retval;
}

void gfx::VkConstantBufferPool::Update(ConstantBufferHandle handle, std::uint64_t size, void* data, std::uint32_t frame_idx, std::uint64_t offset)
{
	m_buffers[frame_idx][handle.m_cb_id]->Update(data, size, offset);
}

gfx::DescriptorHeap* gfx::VkConstantBufferPool::GetDescriptorHeap()
{
	return m_desc_heap;
}

void gfx::VkConstantBufferPool::Allocate_Impl(ConstantBufferHandle& handle, std::uint64_t size)
{
	for (std::uint32_t frame_idx = 0; frame_idx < gfx::settings::num_back_buffers; frame_idx++)
	{
		// TODO: memory pool
		auto buffer= new gfx::GPUBuffer(m_context, m_pool, size, gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
		buffer->Map();
		handle.m_cb_set_id = m_desc_heap->CreateSRVFromCB(buffer, m_cb_set_layout, m_binding, frame_idx);
		m_buffers[frame_idx].push_back(buffer);

		// TODO: In theory cb set id and cb id are always the same.
	}
}
