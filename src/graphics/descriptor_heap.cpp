#include "descriptor_heap.hpp"

#include "context.hpp"
#include "gfx_settings.hpp"
#include "root_signature.hpp"
#include "gpu_buffers.hpp"

gfx::DescriptorHeap::DescriptorHeap(Context* context, RootSignature* root_signature)
	: m_context(context), m_descriptor_pool(VK_NULL_HANDLE), m_descriptor_pool_create_info()
{
	auto logical_device = m_context->m_logical_device;
	const auto num_back_buffers = gfx::settings::num_back_buffers;

	// Create the descriptor pool
	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = num_back_buffers;

	m_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	m_descriptor_pool_create_info.poolSizeCount = 1;
	m_descriptor_pool_create_info.pPoolSizes = &pool_size;
	m_descriptor_pool_create_info.maxSets = num_back_buffers;

	if (vkCreateDescriptorPool(logical_device, &m_descriptor_pool_create_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}

	// Create the descriptor tables
	std::vector<VkDescriptorSetLayout> layouts(num_back_buffers, root_signature->m_descriptor_set_layouts[0]);
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pool;
	alloc_info.descriptorSetCount = num_back_buffers;
	alloc_info.pSetLayouts = layouts.data();

	m_descriptor_sets.resize(num_back_buffers);
	if (vkAllocateDescriptorSets(logical_device, &alloc_info, m_descriptor_sets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}

gfx::DescriptorHeap::~DescriptorHeap()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyDescriptorPool(logical_device, m_descriptor_pool, nullptr);
}

void gfx::DescriptorHeap::CreateSRVFromCB(GPUBuffer* buffer, std::uint32_t handle)
{
	auto logical_device = m_context->m_logical_device;

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = buffer->m_buffer;
	buffer_info.offset = 0;
	buffer_info.range = buffer->m_size;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[handle];
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pBufferInfo = &buffer_info;
	descriptor_write.pImageInfo = nullptr; // Optional
	descriptor_write.pTexelBufferView = nullptr; // Optional
	vkUpdateDescriptorSets(logical_device, 1, &descriptor_write, 0, nullptr);
}