#include "descriptor_heap.hpp"

#include "context.hpp"
#include "gfx_settings.hpp"
#include "root_signature.hpp"
#include "gpu_buffers.hpp"
#include "render_target.hpp"
#include "../util/log.hpp"

// we always create 1 pool as a heap
// in the pool we create sets for the different type of descriptors
// in these sets we create multiple descriptors of the same type as the corresponding set.

gfx::DescriptorHeap::DescriptorHeap(Context* context, Desc desc)
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
	m_descriptor_pool_create_info.maxSets = desc.m_num_descriptors;

	m_descriptor_sets.resize(desc.m_versions);

	for (auto version = 0u; version < desc.m_versions; version++)
	{
		if (vkCreateDescriptorPool(logical_device, &m_descriptor_pool_create_info, nullptr, &m_descriptor_pools[version]) !=
		    VK_SUCCESS)
		{
			LOGC("failed to create descriptor pool!");
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

std::uint32_t gfx::DescriptorHeap::CreateSRVFromCB(GPUBuffer* buffer, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx)
{
	return CreateSRVFromCB(buffer, root_signature->m_descriptor_set_layouts[handle], handle, frame_idx);
}

std::uint32_t gfx::DescriptorHeap::CreateSRVFromCB(GPUBuffer* buffer, VkDescriptorSetLayout layout, std::uint32_t handle, std::uint32_t frame_idx)
{
	auto logical_device = m_context->m_logical_device;

	// Create the descriptor sets
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pools[frame_idx];
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;

	VkDescriptorSet descriptor_set;
	if (vkAllocateDescriptorSets(logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
	{
		LOGC("failed to allocate descriptor sets!");
	}
	m_descriptor_sets[frame_idx].push_back(descriptor_set);

	auto buffer_info = new VkDescriptorBufferInfo();
	// Command list will destroy it later.
	buffer_info->buffer = buffer->m_buffer;
	buffer_info->offset = 0;
	buffer_info->range = buffer->m_size;

	auto descriptor_set_id = m_descriptor_sets[frame_idx].size() - 1;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][descriptor_set_id];  // TODO: Don't use 0 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pBufferInfo = buffer_info;
	descriptor_write.pImageInfo = nullptr;
	descriptor_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(logical_device, 1u, &descriptor_write, 0, nullptr);

	return descriptor_set_id;
}

std::uint32_t gfx::DescriptorHeap::CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx, SamplerDesc sampler_desc)
{
	return CreateSRVSetFromTexture(texture, root_signature->m_descriptor_set_layouts[handle], handle, frame_idx, sampler_desc);
}


std::uint32_t gfx::DescriptorHeap::CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, VkDescriptorSetLayout layout, std::uint32_t handle, std::uint32_t frame_idx, SamplerDesc sampler_desc)
{
	auto logical_device = m_context->m_logical_device;

	// Create the descriptor sets
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pools[frame_idx];
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;

	VkDescriptorSet descriptor_set;
	if (vkAllocateDescriptorSets(logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
	{
		LOGC("failed to allocate descriptor sets!");
	}
	m_descriptor_sets[frame_idx].push_back(descriptor_set);

	auto new_sampler = CreateSampler(sampler_desc);
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

	auto descriptor_set_id = m_descriptor_sets[frame_idx].size() - 1;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][descriptor_set_id]; // TODO: Don't use 1 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.descriptorCount = image_infos.size();
	descriptor_write.pBufferInfo = nullptr;
	descriptor_write.pImageInfo = image_infos.data();
	descriptor_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(logical_device, 1u, &descriptor_write, 0, nullptr);

	return descriptor_set_id;
}

std::uint32_t gfx::DescriptorHeap::CreateUAVSetFromTexture(std::vector<Texture*> texture, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx, SamplerDesc sampler_desc)
{
	auto logical_device = m_context->m_logical_device;

	// Create the descriptor sets
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pools[frame_idx];
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &root_signature->m_descriptor_set_layouts[handle];

	VkDescriptorSet descriptor_set;
	if (vkAllocateDescriptorSets(logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
	{
		LOGC("failed to allocate descriptor sets!");
	}
	m_descriptor_sets[frame_idx].push_back(descriptor_set);

	auto new_sampler = CreateSampler(sampler_desc);
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

	auto descriptor_set_id = m_descriptor_sets[frame_idx].size() - 1;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][descriptor_set_id]; // TODO: Don't use 1 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.descriptorCount = image_infos.size();
	descriptor_write.pBufferInfo = nullptr;
	descriptor_write.pImageInfo = image_infos.data();
	descriptor_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(logical_device, 1u, &descriptor_write, 0, nullptr);

	return descriptor_set_id;
}


std::uint32_t gfx::DescriptorHeap::CreateSRVSetFromRT(RenderTarget* render_target, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx, bool include_depth, std::optional<SamplerDesc> sampler_desc)
{
	auto logical_device = m_context->m_logical_device;

	// Create the descriptor sets
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pools[frame_idx];
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &root_signature->m_descriptor_set_layouts[handle];

	VkDescriptorSet descriptor_set;
	if (vkAllocateDescriptorSets(logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
	{
		LOGC("failed to allocate descriptor sets!");
	}
	m_descriptor_sets[frame_idx].push_back(descriptor_set);

	VkSampler new_sampler = VK_NULL_HANDLE;
	if (sampler_desc.has_value())
	{
		new_sampler = CreateSampler(sampler_desc.value());
		m_image_samplers.push_back(new_sampler);
	}

	std::vector<VkDescriptorImageInfo> image_infos;

	for (std::size_t i = 0; i < render_target->m_desc.m_rtv_formats.size(); i++)
	{
		// image view
		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = render_target->m_images[i];
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = render_target->m_desc.m_rtv_formats[i];
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
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
		image_info.imageLayout = sampler_desc.has_value() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
		image_info.imageView = new_view;
		image_info.sampler = new_sampler;
		image_infos.push_back(image_info);
	}

	// Optional Depth
	if (include_depth && render_target->m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		// image view
		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = render_target->m_depth_buffer;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = render_target->m_desc.m_depth_format;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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
		image_info.imageLayout = sampler_desc.has_value() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
		image_info.imageView = new_view;
		image_info.sampler = new_sampler;
		image_infos.push_back(image_info);
	}

	auto descriptor_set_id = m_descriptor_sets[frame_idx].size() - 1;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // TODO: Descriptor set id can just be ::back here.
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][descriptor_set_id]; // TODO: Don't use 1 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = sampler_desc.has_value() ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptor_write.descriptorCount = image_infos.size();
	descriptor_write.pBufferInfo = nullptr;
	descriptor_write.pImageInfo = image_infos.data();
	descriptor_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(logical_device, 1u, &descriptor_write, 0, nullptr);

	return descriptor_set_id;
}

std::uint32_t gfx::DescriptorHeap::CreateUAVSetFromRT(RenderTarget* render_target, std::uint32_t rt_idx, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx, SamplerDesc sampler_desc)
{
	auto logical_device = m_context->m_logical_device;

	// Create the descriptor sets
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pools[frame_idx];
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &root_signature->m_descriptor_set_layouts[handle];

	VkDescriptorSet descriptor_set;
	if (vkAllocateDescriptorSets(logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
	{
		LOGC("failed to allocate descriptor sets!");
	}
	m_descriptor_sets[frame_idx].push_back(descriptor_set);

	auto new_sampler = CreateSampler(sampler_desc);
	m_image_samplers.push_back(new_sampler);

	std::vector<VkDescriptorImageInfo> image_infos;

	// image view
	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = render_target->m_images[rt_idx];
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = render_target->m_desc.m_rtv_formats[rt_idx];
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
	image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	image_info.imageView = new_view;
	image_info.sampler = new_sampler;
	image_infos.push_back(image_info);

	auto descriptor_set_id = m_descriptor_sets[frame_idx].size() - 1;

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // TODO: Descriptor set id can just be ::back here.
	descriptor_write.dstSet = m_descriptor_sets[frame_idx][descriptor_set_id]; // TODO: Don't use 1 but get the set that corresponds to the correct descriptor type.
	descriptor_write.dstBinding = handle;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptor_write.descriptorCount = image_infos.size();
	descriptor_write.pBufferInfo = nullptr;
	descriptor_write.pImageInfo = image_infos.data();
	descriptor_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(logical_device, 1u, &descriptor_write, 0, nullptr);

	return descriptor_set_id;
}


VkSampler gfx::DescriptorHeap::CreateSampler(SamplerDesc sampler_desc)
{
	auto logical_device = m_context->m_logical_device;

	// sampler
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	switch(sampler_desc.m_filter)
	{
		case enums::TextureFilter::FILTER_ANISOTROPIC:
		case enums::TextureFilter::FILTER_LINEAR:
			sampler_info.magFilter = VK_FILTER_LINEAR;
			sampler_info.minFilter = VK_FILTER_LINEAR;
			sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			break;
		case enums::TextureFilter::FILTER_LINEAR_POINT:
			sampler_info.magFilter = VK_FILTER_LINEAR;
			sampler_info.minFilter = VK_FILTER_NEAREST;
			sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			break;
		case enums::TextureFilter::FILTER_POINT:
			sampler_info.magFilter = VK_FILTER_NEAREST;
			sampler_info.minFilter = VK_FILTER_NEAREST;
			sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		default: LOGW("Tried Creating a sampler with a unsupported type"); break;
	}
	sampler_info.addressModeU = (VkSamplerAddressMode)sampler_desc.m_address_mode;
	sampler_info.addressModeV = (VkSamplerAddressMode)sampler_desc.m_address_mode;
	sampler_info.addressModeW = (VkSamplerAddressMode)sampler_desc.m_address_mode;
	sampler_info.anisotropyEnable = sampler_desc.m_filter == enums::TextureFilter::FILTER_ANISOTROPIC ? VK_TRUE : VK_FALSE;
	sampler_info.maxAnisotropy = 16; // TODO UNHARDCODE AND QUERY FOR SUPPORT
	sampler_info.borderColor = (VkBorderColor)sampler_desc.m_border_color;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkSampler new_sampler;
	if (vkCreateSampler(logical_device, &sampler_info, nullptr, &new_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	return new_sampler;
}