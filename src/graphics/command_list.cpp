/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "command_list.hpp"

#include "context.hpp"
#include "gfx_settings.hpp"
#include "render_target.hpp"
#include "pipeline_state.hpp"

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
	m_cmd_pool_create_info.flags = 0;

	if (vkCreateCommandPool(logical_device, &m_cmd_pool_create_info, nullptr, &m_cmd_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
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

	if (vkBeginCommandBuffer(m_cmd_buffers[frame_idx], &begin_info) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
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
}

void gfx::CommandList::BindPipelineState(gfx::PipelineState* pipeline, std::uint32_t frame_idx)
{
	vkCmdBindPipeline(m_cmd_buffers[frame_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->m_pipeline);
}

void gfx::CommandList::DrawInstanced(std::uint32_t frame_idx, std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance)
{
	vkCmdDraw(m_cmd_buffers[frame_idx], vertex_count, instance_count, first_vertex, first_instance);
}

void gfx::CommandList::Close(std::uint32_t frame_idx)
{
	vkCmdEndRenderPass(m_cmd_buffers[frame_idx]);

	if (vkEndCommandBuffer(m_cmd_buffers[frame_idx]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}
