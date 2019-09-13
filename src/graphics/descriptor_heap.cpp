#include "descriptor_heap.hpp"

#include "context.hpp"
#include "gfx_settings.hpp"
#include "root_signature.hpp"
#include "gpu_buffers.hpp"
#include "../util/log.hpp"

// we always create 1 pool as a heap
// in the pool we create sets for the different type of descriptors
// in these sets we create multiple descriptors of the same type as the corresponding set.

gfx::DescriptorHeap::DescriptorHeap(Context* context, RootSignature* root_signature, Desc desc)
	: m_context(context), m_desc(desc), m_descriptor_pool_create_info()
{
	auto logical_device = m_context->m_logical_device;

	m_descriptor_pools.resize(desc.m_versions);

	// Create the descriptor pool
	std::vector<VkDescriptorPoolSize> pool_sizes(2);
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = desc.m_num_descriptors; // TODO: This wastes space. But gets us closer to DX12 behaviour
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount =  desc.m_num_descriptors; // TODO: This wastes space. But gets us closer to DX12 behaviour

	m_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	m_descriptor_pool_create_info.poolSizeCount = pool_sizes.size();
	m_descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
	m_descriptor_pool_create_info.maxSets = pool_sizes.size();

	m_descriptor_sets.resize(desc.m_versions);
	m_queued_writes.resize(desc.m_versions);

	for (auto version = 0u; version < desc.m_versions; version++)
	{
		if (vkCreateDescriptorPool(logical_device, &m_descriptor_pool_create_info, nullptr, &m_descriptor_pools[version]) !=
		    VK_SUCCESS)
		{
			LOGC("failed to create descriptor pool!");
		}

		if (!root_signature) continue;

		// Create the descriptor tables
		std::vector<VkDescriptorSetLayout> layouts(root_signature->m_descriptor_set_layouts.size());
		for (auto i = 0u; i < root_signature->m_descriptor_set_layouts.size(); i++)
		{
			layouts[i] = root_signature->m_descriptor_set_layouts[i];
		}

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = m_descriptor_pools[version];
		alloc_info.descriptorSetCount = pool_sizes.size();
		alloc_info.pSetLayouts = layouts.data();

		m_descriptor_sets[version].resize(pool_sizes.size());
		if (vkAllocateDescriptorSets(logical_device, &alloc_info, m_descriptor_sets[version].data()) != VK_SUCCESS)
		{
			LOGC("failed to allocate descriptor sets!");
		}
	}
}

gfx::DescriptorHeap::~DescriptorHeap()
{
	auto logical_device = m_context->m_logical_device;

	for (auto& pool : m_descriptor_pools)
	{
		vkDestroyDescriptorPool(logical_device, pool, nullptr);
	}

	for (auto& view : m_image_views)
	{
		vkDestroyImageView(logical_device, view, nullptr);
	}

	for (auto& sampler : m_image_samplers)
	{
		vkDestroySampler(logical_device, sampler, nullptr);
	}
}

void gfx::DescriptorHeap::CreateSRVFromCB(GPUBuffer* buffer, std::uint32_t handle, std::uint32_t frame_idx)
{
	auto buffer_info = new VkDescriptorBufferInfo();
	// Command list will destroy it later.
	buffer_info->buffer = buffer->m_buffer;
	buffer_info->offset = 0;
	buffer_info->range = buffer->m_size;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][0];  // TODO: Don't use 0 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pBufferInfo = buffer_info;
	descriptor_write.pImageInfo = nullptr;
	descriptor_write.pTexelBufferView = nullptr;

	m_queued_writes[frame_idx].push_back(descriptor_write);
}


void gfx::DescriptorHeap::CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, std::uint32_t handle, std::uint32_t frame_idx)
{
	auto logical_device = m_context->m_logical_device;

	// sampler
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16; // TODO UNHARDCODE AND QUERY FOR SUPPORT
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkSampler new_sampler;
	if (vkCreateSampler(logical_device, &sampler_info, nullptr, &new_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	m_image_samplers.push_back(new_sampler);

	std::vector<VkDescriptorImageInfo> image_infos;

	for (auto& t : texture)
	{
		// image view
		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = t->m_texture;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = t->m_desc.m_format;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;

		VkImageView new_view;
		if (vkCreateImageView(logical_device, &view_info, nullptr, &new_view) != VK_SUCCESS)
		{
			LOGC("Failed to create texture image view!");
		}
		m_image_views.push_back(new_view);

		VkDescriptorImageInfo image_info = {};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = new_view;
		image_info.sampler = new_sampler;
		image_infos.push_back(image_info);
	}

	void* final_info = malloc(sizeof(VkDescriptorImageInfo) * image_infos.size());
	std::memcpy(final_info, image_infos.data(), sizeof(VkDescriptorImageInfo) * image_infos.size());

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][1]; // TODO: Don't use 1 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.descriptorCount = image_infos.size();
	descriptor_write.pBufferInfo = nullptr;
	descriptor_write.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(final_info);
	descriptor_write.pTexelBufferView = nullptr;

	m_queued_writes[frame_idx].push_back(descriptor_write);
}

void gfx::DescriptorHeap::CreateSRVFromTexture(StagingTexture* texture, std::uint32_t handle, std::uint32_t frame_idx)
{
	auto logical_device = m_context->m_logical_device;

	// image view
	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = texture->m_texture;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = texture->m_desc.m_format;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VkImageView new_view;
	if (vkCreateImageView(logical_device, &view_info, nullptr, &new_view) != VK_SUCCESS)
	{
		LOGC("Failed to create texture image view!");
	}
	m_image_views.push_back(new_view);

	// sampler
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16; // TODO UNHARDCODE AND QUERY FOR SUPPORT
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkSampler new_sampler;
	if (vkCreateSampler(logical_device, &sampler_info, nullptr, &new_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	m_image_samplers.push_back(new_sampler);

	auto image_info = new VkDescriptorImageInfo();
	// Command list will destroy it later.
	image_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info->imageView = new_view;
	image_info->sampler = new_sampler;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][1]; // TODO: Don't use 1 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.descriptorCount = 2;
	descriptor_write.pBufferInfo = nullptr;
	descriptor_write.pImageInfo = image_info;
	descriptor_write.pTexelBufferView = nullptr;

	m_queued_writes[frame_idx].push_back(descriptor_write);
}