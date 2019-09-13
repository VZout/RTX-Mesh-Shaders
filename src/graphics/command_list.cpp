/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "command_list.hpp"

#include "../util/log.hpp"
#include "context.hpp"
#include "gfx_settings.hpp"
#include "descriptor_heap.hpp"
#include "render_target.hpp"
#include "pipeline_state.hpp"
#include "root_signature.hpp"
#include "gpu_buffers.hpp"

gfx::CommandList::CommandList(CommandQueue* queue)
	: m_context(queue->m_context), m_queue(queue), m_cmd_pool(VK_NULL_HANDLE), m_cmd_pool_create_info()
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
	m_bound_render_target = std::vector<bool>(gfx::settings::num_back_buffers, false);

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
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	vkResetCommandBuffer(m_cmd_buffers[frame_idx], 0);

	if (vkResetCommandBuffer(m_cmd_buffers[frame_idx], 0) != VK_SUCCESS ||
		vkBeginCommandBuffer(m_cmd_buffers[frame_idx], &begin_info) != VK_SUCCESS)
	{
		LOGC("failed to begin recording command buffer!");
	}
}

void gfx::CommandList::Close(std::uint32_t frame_idx)
{
	if (m_bound_render_target[frame_idx])
	{
		vkCmdEndRenderPass(m_cmd_buffers[frame_idx]);
		m_bound_render_target[frame_idx] = false;
	}

	if (vkEndCommandBuffer(m_cmd_buffers[frame_idx]) != VK_SUCCESS)
	{
		LOGC("failed to record command buffer!");
	}
}

void gfx::CommandList::BindRenderTargetVersioned(RenderTarget* render_target, std::uint32_t frame_idx)
{
	std::array<VkClearValue, 2> clear_values = {};
	clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = render_target->m_render_pass;
	render_pass_begin_info.framebuffer = render_target->m_frame_buffers[frame_idx];
	render_pass_begin_info.renderArea.offset = {0, 0};
	render_pass_begin_info.renderArea.extent.width = render_target->GetWidth();
	render_pass_begin_info.renderArea.extent.height = render_target->GetHeight();
	render_pass_begin_info.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(m_cmd_buffers[frame_idx], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	m_bound_render_target[frame_idx] = true;
}

void gfx::CommandList::BindPipelineState(gfx::PipelineState* pipeline, std::uint32_t frame_idx)
{
	vkCmdBindPipeline(m_cmd_buffers[frame_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_pipeline);
}

void gfx::CommandList::BindVertexBuffer(StagingBuffer* staging_buffer, std::uint32_t frame_idx)
{
	std::vector<VkBuffer> buffers = { staging_buffer->m_buffer };
	std::vector<VkDeviceSize> offsets = { 0 };
	vkCmdBindVertexBuffers(m_cmd_buffers[frame_idx], 0, buffers.size(), buffers.data(), offsets.data());
}

void gfx::CommandList::BindIndexBuffer(StagingBuffer* staging_buffer, std::uint32_t frame_idx)
{
	VkDeviceSize offset = { 0 };
	vkCmdBindIndexBuffer(m_cmd_buffers[frame_idx], staging_buffer->m_buffer, offset, VkIndexType::VK_INDEX_TYPE_UINT32);
}

void gfx::CommandList::BindDescriptorHeap(RootSignature* root_signature, DescriptorHeap* heap, std::uint32_t frame_idx)
{
	if (!heap->m_queued_writes[frame_idx].empty())
	{
		auto logical_device = m_context->m_logical_device;
		vkUpdateDescriptorSets(logical_device, static_cast<std::uint32_t>(2), heap->m_queued_writes[frame_idx].data(), 0, nullptr);

		// destroy the buffer or image infos we allocated some where else
		for (auto& write : heap->m_queued_writes[frame_idx])
		{
			if (write.pImageInfo) delete write.pImageInfo;
			if (write.pBufferInfo) delete write.pBufferInfo;
		}

		heap->m_queued_writes[frame_idx].clear();
	}

	vkCmdBindDescriptorSets(m_cmd_buffers[frame_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, root_signature->m_pipeline_layout,
			0, heap->m_descriptor_sets[frame_idx].size(), heap->m_descriptor_sets[frame_idx].data(), 0, nullptr);
}

void gfx::CommandList::StageBuffer(StagingBuffer* staging_buffer, std::uint32_t frame_idx)
{
	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = staging_buffer->m_size;
	vkCmdCopyBuffer(m_cmd_buffers[frame_idx], staging_buffer->m_staging_buffer, staging_buffer->m_buffer, 1, &copy_region);
}

void gfx::CommandList::StageTexture(StagingTexture* texture, std::uint32_t frame_idx)
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
		m_cmd_buffers[frame_idx],
		texture->m_buffer,
		texture->m_texture,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

}

void gfx::CommandList::TransitionDepth(RenderTarget* render_target, VkImageLayout from, VkImageLayout to, std::uint32_t frame_idx)
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
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

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
			m_cmd_buffers[frame_idx],
			source_stage, destination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
	);
}

// TODO: Duplicate of transition depth
void gfx::CommandList::TransitionTexture(StagingTexture* texture, VkImageLayout from, VkImageLayout to, std::uint32_t frame_idx)
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
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

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
			m_cmd_buffers[frame_idx],
			source_stage, destination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
	);
}

void gfx::CommandList::Draw(std::uint32_t frame_idx, std::uint32_t vertex_count, std::uint32_t instance_count,
		std::uint32_t first_vertex, std::uint32_t first_instance)
{
	vkCmdDraw(m_cmd_buffers[frame_idx], vertex_count, instance_count, first_vertex, first_instance);
}

void gfx::CommandList::DrawIndexed(std::uint32_t frame_idx, std::uint32_t idx_count, std::uint32_t instance_count,
		std::uint32_t first_idx, std::uint32_t vertex_offset, std::uint32_t first_instance)
{
	vkCmdDrawIndexed(m_cmd_buffers[frame_idx], idx_count, instance_count, first_idx, vertex_offset, first_instance);
}