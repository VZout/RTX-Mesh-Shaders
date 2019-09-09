/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "command_list.hpp"

#include "context.hpp"
#include "gfx_settings.hpp"
#include "descriptor_heap.hpp"
#include "render_target.hpp"
#include "pipeline_state.hpp"
#include "root_signature.hpp"

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
			throw std::runtime_error("Tried to create a command queue with a unsupported type");
	}

	m_cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	m_cmd_pool_create_info.queueFamilyIndex = m_context->GetDirectQueueFamilyIdx();
	m_cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(logical_device, &m_cmd_pool_create_info, nullptr, &m_cmd_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
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
		throw std::runtime_error("failed to allocate command buffers!");
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
		throw std::runtime_error("failed to begin recording command buffer!");
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
		throw std::runtime_error("failed to record command buffer!");
	}
}

void gfx::CommandList::BindRenderTargetVersioned(RenderTarget* render_target, std::uint32_t frame_idx)
{
	VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = render_target->m_render_pass;
	render_pass_begin_info.framebuffer = render_target->m_frame_buffers[frame_idx];
	render_pass_begin_info.renderArea.offset = {0, 0};
	render_pass_begin_info.renderArea.extent.width = render_target->GetWidth();
	render_pass_begin_info.renderArea.extent.height = render_target->GetHeight();
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &clearColor;

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

void gfx::CommandList::BindDescriptorTable(RootSignature* root_signature, DescriptorHeap* heap, std::uint32_t handle, std::uint32_t frame_idx)
{
	vkCmdBindDescriptorSets(m_cmd_buffers[frame_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, root_signature->m_pipeline_layout, 0, 1, &heap->m_descriptor_sets[handle], 0, nullptr);
}

void gfx::CommandList::StageBuffer(StagingBuffer* staging_buffer, std::uint32_t frame_idx)
{
	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = staging_buffer->m_size;
	vkCmdCopyBuffer(m_cmd_buffers[frame_idx], staging_buffer->m_staging_buffer, staging_buffer->m_buffer, 1, &copy_region);
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