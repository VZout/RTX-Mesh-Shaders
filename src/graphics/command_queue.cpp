/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "command_queue.hpp"

#include "context.hpp"

gfx::CommandQueue::CommandQueue(Context* context, CommandQueueType queue_type)
	: m_queue(VK_NULL_HANDLE)
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
