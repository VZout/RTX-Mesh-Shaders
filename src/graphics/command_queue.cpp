/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "command_queue.hpp"

#include "command_list.hpp"
#include "context.hpp"
#include "fence.hpp"

gfx::CommandQueue::CommandQueue(Context* context, CommandQueueType queue_type)
	: m_queue(VK_NULL_HANDLE), m_context(context), m_type(queue_type)
{
	std::uint32_t queue_family_idx = 0;

	switch(queue_type)
	{
		case CommandQueueType::DIRECT:
			queue_family_idx = context->GetDirectQueueFamilyIdx();
			break;
		default:
			throw std::runtime_error("Tried to create a command queue with a unsupported type");
	}

	vkGetDeviceQueue(context->m_logical_device, queue_family_idx, 0, &m_queue);
}

void gfx::CommandQueue::Execute(std::vector<CommandList*> cmd_lists, Fence* fence, std::uint32_t frame_idx)
{
	std::vector<VkSemaphore> signal_semaphores;
	std::vector<VkSemaphore> wait_semaphores;
	std::vector<VkPipelineStageFlags> wait_stages;
	std::vector<VkCommandBuffer> cmd_buffers(cmd_lists.size());
	for (std::size_t i = 0; i < cmd_lists.size(); i++)
	{
		cmd_buffers[i] = cmd_lists[i]->m_cmd_buffers[frame_idx];
	}

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = cmd_buffers.size();
	submit_info.pCommandBuffers = cmd_buffers.data();

	if (fence)
	{
		signal_semaphores = { fence->m_signal_semaphore };
		wait_semaphores = { fence->m_wait_semaphore };
		wait_stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		submit_info.waitSemaphoreCount = wait_semaphores.size();
		submit_info.pWaitSemaphores = wait_semaphores.data();
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.signalSemaphoreCount = signal_semaphores.size();
		submit_info.pSignalSemaphores = signal_semaphores.data();
	}

	if (vkQueueSubmit(m_queue, 1, &submit_info, fence ? fence->m_fence : VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

void gfx::CommandQueue::Wait()
{
	vkQueueWaitIdle(m_queue);
}
