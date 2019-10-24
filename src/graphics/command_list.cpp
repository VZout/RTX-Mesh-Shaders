/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "command_list.hpp"

#include <array>

#include "../util/log.hpp"
#include "context.hpp"
#include "gfx_settings.hpp"
#include "descriptor_heap.hpp"
#include "render_target.hpp"
#include "pipeline_state.hpp"
#include "root_signature.hpp"
#include "gpu_buffers.hpp"
#include "render_window.hpp"

gfx::CommandList::CommandList(CommandQueue* queue)
	: m_context(queue->m_context), m_queue(queue), m_cmd_pool(VK_NULL_HANDLE), m_cmd_pool_create_info(), m_frame_idx(0)
{
	// Create the command pool
	auto logical_device = m_context->m_logical_device;
	std::uint32_t queue_family_idx = 0;

	switch(queue->m_type)
	{
		case CommandQueueType::DIRECT:
			queue_family_idx = m_context->GetDirectQueueFamilyIdx();
			break;
		default:
			LOGC("Tried to create a command queue with a unsupported type");
	}

	m_cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	m_cmd_pool_create_info.queueFamilyIndex = queue_family_idx;
	m_cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(logical_device, &m_cmd_pool_create_info, nullptr, &m_cmd_pool) != VK_SUCCESS)
	{
		LOGC("Failed to create command pool.");
	}

	// Create the actual command buffers
	m_cmd_buffers.resize(gfx::settings::num_back_buffers);

	VkCommandBufferAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocation_info.commandPool = m_cmd_pool;
	allocation_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocation_info.commandBufferCount = (std::uint32_t) m_cmd_buffers.size();

	if (vkAllocateCommandBuffers(logical_device, &allocation_info, m_cmd_buffers.data()) != VK_SUCCESS)
	{
		LOGC("Failed to allocate command buffers.");
	}
}

gfx::CommandList::~CommandList()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyCommandPool(logical_device, m_cmd_pool, nullptr);
}

void gfx::CommandList::Begin(std::uint32_t frame_idx)
{
	m_frame_idx = frame_idx;

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	//vkResetCommandBuffer(m_cmd_buffers[frame_idx], 0);

	if (vkResetCommandBuffer(m_cmd_buffers[frame_idx], 0) != VK_SUCCESS ||
		vkBeginCommandBuffer(m_cmd_buffers[frame_idx], &begin_info) != VK_SUCCESS)
	{
		LOGC("failed to begin recording command buffer!");
	}
}

void gfx::CommandList::Close()
{
	if (vkEndCommandBuffer(m_cmd_buffers[m_frame_idx]) != VK_SUCCESS)
	{
		LOGC("failed to record command buffer!");
	}
}

void gfx::CommandList::BindRenderTargetVersioned(RenderTarget* render_target)
{
	std::array<VkClearValue, 2> clear_values = {};
	clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = render_target->m_render_pass;
	render_pass_begin_info.framebuffer = render_target->m_frame_buffers[m_frame_idx];
	render_pass_begin_info.renderArea.offset = {0, 0};
	render_pass_begin_info.renderArea.extent.width = render_target->GetWidth();
	render_pass_begin_info.renderArea.extent.height = render_target->GetHeight();
	render_pass_begin_info.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(m_cmd_buffers[m_frame_idx], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void gfx::CommandList::BindRenderTarget(RenderTarget* render_target)
{
	auto num_render_targets = render_target->m_images.size();
	std::vector<VkClearValue> clear_values(num_render_targets + 1);
	for (std::size_t i = 0; i < num_render_targets; i++)
	{
		clear_values[i].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	}
	clear_values.back().depthStencil = {1.0f, 0 };

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = render_target->m_render_pass;
	render_pass_begin_info.framebuffer = render_target->m_frame_buffers[0];
	render_pass_begin_info.renderArea.offset = {0, 0};
	render_pass_begin_info.renderArea.extent.width = render_target->GetWidth();
	render_pass_begin_info.renderArea.extent.height = render_target->GetHeight();
	render_pass_begin_info.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(m_cmd_buffers[m_frame_idx], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void gfx::CommandList::UnbindRenderTarget()
{
	vkCmdEndRenderPass(m_cmd_buffers[m_frame_idx]);
}

void gfx::CommandList::BindPipelineState(gfx::PipelineState* pipeline)
{
	vkCmdBindPipeline(m_cmd_buffers[m_frame_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_pipeline);
}

void gfx::CommandList::BindComputePipelineState(gfx::PipelineState* pipeline)
{
	vkCmdBindPipeline(m_cmd_buffers[m_frame_idx], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->m_pipeline);
}

void gfx::CommandList::BindVertexBuffer(StagingBuffer* staging_buffer)
{
	std::vector<VkBuffer> buffers = { staging_buffer->m_buffer };
	std::vector<VkDeviceSize> offsets = { 0 };
	vkCmdBindVertexBuffers(m_cmd_buffers[m_frame_idx], 0, buffers.size(), buffers.data(), offsets.data());
}

void gfx::CommandList::BindIndexBuffer(StagingBuffer* staging_buffer)
{
	VkDeviceSize offset = { 0 };
	vkCmdBindIndexBuffer(m_cmd_buffers[m_frame_idx], staging_buffer->m_buffer, offset, staging_buffer->m_stride == 4 ? VkIndexType::VK_INDEX_TYPE_UINT32 : VkIndexType::VK_INDEX_TYPE_UINT16);
}

void gfx::CommandList::BindDescriptorHeap(RootSignature* root_signature, std::vector<std::pair<DescriptorHeap*, std::uint32_t>> sets)
{
	std::vector<VkDescriptorSet> descriptor_sets(sets.size());

	for (std::size_t i = 0; i < sets.size(); i++)
	{
		auto versions = sets[i].first->m_descriptor_sets.size();
		descriptor_sets[i] = sets[i].first->m_descriptor_sets[m_frame_idx % versions][sets[i].second];
	}

	vkCmdBindDescriptorSets(m_cmd_buffers[m_frame_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, root_signature->m_pipeline_layout,
	                        0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
}

// TODO: Duplicate with binddescriptorheap
void gfx::CommandList::BindComputeDescriptorHeap(RootSignature* root_signature, std::vector<std::pair<DescriptorHeap*, std::uint32_t>> sets)
{
	std::vector<VkDescriptorSet> descriptor_sets(sets.size());

	for (std::size_t i = 0; i < sets.size(); i++)
	{
		auto versions = sets[i].first->m_descriptor_sets.size();
		descriptor_sets[i] = sets[i].first->m_descriptor_sets[m_frame_idx % versions][sets[i].second];
	}

	vkCmdBindDescriptorSets(m_cmd_buffers[m_frame_idx], VK_PIPELINE_BIND_POINT_COMPUTE, root_signature->m_pipeline_layout,
	                        0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
}

void gfx::CommandList::BindComputePushConstants(RootSignature* root_signature, void* data, std::uint32_t size)
{
	vkCmdPushConstants(m_cmd_buffers[m_frame_idx], root_signature->m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);
}

void gfx::CommandList::StageBuffer(StagingBuffer* staging_buffer)
{
	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = staging_buffer->m_size;
	vkCmdCopyBuffer(m_cmd_buffers[m_frame_idx], staging_buffer->m_staging_buffer, staging_buffer->m_buffer, 1, &copy_region);
}

void gfx::CommandList::StageTexture(StagingTexture* texture)
{
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {
		texture->m_desc.m_width,
		texture->m_desc.m_height,
		1
	};

	vkCmdCopyBufferToImage(
		m_cmd_buffers[m_frame_idx],
		texture->m_buffer,
		texture->m_texture,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void gfx::CommandList::CopyRenderTargetToRenderWindow(RenderTarget* render_target, std::uint32_t rt_idx, RenderWindow* render_window)
{
	VkImageSubresourceLayers source_target_layers = {};
	source_target_layers.mipLevel = 0;
	source_target_layers.layerCount = 1;
	source_target_layers.baseArrayLayer = 0;
	source_target_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageCopy region = {};
	region.dstOffset = { 0, 0, 0 };
	region.srcOffset = { 0, 0, 0 };
	region.dstSubresource = source_target_layers;
	region.srcSubresource = source_target_layers;
	region.extent = {
			render_target->GetWidth(),
			render_target->GetHeight(),
			1
	};

	vkCmdCopyImage(
			m_cmd_buffers[m_frame_idx],
			render_target->m_images[rt_idx],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			render_window->m_images[m_frame_idx],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
	);
}

void gfx::CommandList::TransitionDepth(RenderTarget* render_target, VkImageLayout from, VkImageLayout to)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = render_target->m_depth_buffer;

	if (to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (enums::FormatHasStencilComponent(render_target->m_depth_buffer_create_info.format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = render_target->m_desc.m_mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;


	if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_GENERAL && to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && to == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // even though its general it still needs the depth bit due to the format

		source_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else
	{
		LOGE("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
			m_cmd_buffers[m_frame_idx],
			source_stage, destination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
	);
}

// TODO: Duplicate of transition depth
void gfx::CommandList::TransitionTexture(StagingTexture* texture, VkImageLayout from, VkImageLayout to)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture->m_texture;

	if (to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (enums::FormatHasStencilComponent(texture->m_desc.m_format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = texture->m_desc.m_mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = texture->m_desc.m_array_size;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		LOGE("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
			m_cmd_buffers[m_frame_idx],
			source_stage, destination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
	);
}

void gfx::CommandList::TransitionRenderTarget(RenderTarget* render_target, VkImageLayout from, VkImageLayout to)
{
	for (std::size_t i = 0; i < render_target->m_images.size(); i++)
	{
		TransitionRenderTarget(render_target, i, from, to);
	}
}

// TODO: Duplicate of transition depth
void gfx::CommandList::TransitionRenderTarget(RenderTarget* render_target, std::uint32_t rt_idx, VkImageLayout from, VkImageLayout to)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = render_target->m_images[rt_idx];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = render_target->m_desc.m_mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = render_target->m_desc.m_is_cube_map ? 6 : 1;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_GENERAL && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && to == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_GENERAL && to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_GENERAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;

		source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = 0;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && to == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		LOGE("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
			m_cmd_buffers[m_frame_idx],
			source_stage, destination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
	);
}

// Note that it transitions it to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
void gfx::CommandList::GenerateMipMap(gfx::Texture* texture)
{
	GenerateMipMap(texture->m_texture, texture->m_desc.m_format, texture->m_desc.m_width, texture->m_desc.m_height, texture->m_desc.m_mip_levels, texture->m_desc.m_array_size);
}

// Note that it transitions it to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
void gfx::CommandList::GenerateMipMap(gfx::RenderTarget* render_target)
{
	for (std::size_t i = 0; i < render_target->m_images.size(); i++)
	{
		GenerateMipMap(render_target->m_images[i], render_target->m_desc.m_rtv_formats[i], render_target->GetWidth(), render_target->GetHeight(), render_target->m_desc.m_mip_levels, render_target->m_desc.m_is_cube_map ? 6 : 1);
	}
}

void gfx::CommandList::GenerateMipMap(VkImage& image, VkFormat format, std::int32_t width, std::int32_t height, std::uint32_t mip_levels, std::uint32_t layers)
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_context->m_physical_device, format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		LOGE("Texture image format does not support linear blitting! Can't generate mipmaps.");
		return;
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layers;
	barrier.subresourceRange.levelCount = 1;

	for (std::uint32_t i = 1; i < mip_levels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(m_cmd_buffers[m_frame_idx],
		                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                     0, nullptr,
		                     0, nullptr,
		                     1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { width, height, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = layers;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { width > 1 ? width / 2 : 1, height > 1 ? height / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = layers;

		vkCmdBlitImage(m_cmd_buffers[m_frame_idx],
		               image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1, &blit,
		               VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(m_cmd_buffers[m_frame_idx],
		                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                     0, nullptr,
		                     0, nullptr,
		                     1, &barrier);

		if (width > 1) width /= 2;
		if (height > 1) height /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(m_cmd_buffers[m_frame_idx],
	                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &barrier);
}

void gfx::CommandList::Draw(std::uint32_t vertex_count, std::uint32_t instance_count,
		std::uint32_t first_vertex, std::uint32_t first_instance)
{
	vkCmdDraw(m_cmd_buffers[m_frame_idx], vertex_count, instance_count, first_vertex, first_instance);
}

void gfx::CommandList::DrawIndexed(std::uint32_t idx_count, std::uint32_t instance_count,
		std::uint32_t first_idx, std::uint32_t vertex_offset, std::uint32_t first_instance)
{
	vkCmdDrawIndexed(m_cmd_buffers[m_frame_idx], idx_count, instance_count, first_idx, vertex_offset, first_instance);
}

void gfx::CommandList::Dispatch(std::uint32_t tg_count_x, std::uint32_t tg_count_y, std::uint32_t tg_count_z)
{
	vkCmdDispatch(m_cmd_buffers[m_frame_idx], tg_count_x, tg_count_y, tg_count_z);
}

void gfx::CommandList::DrawMesh(std::uint32_t count, std::uint32_t first)
{
	m_context->CmdDrawMeshTasksNV(m_cmd_buffers[m_frame_idx], count, first);
}