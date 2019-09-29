/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "fence.hpp"

#include "../util/log.hpp"
#include "context.hpp"

gfx::Fence::Fence(Context* context)
	: m_context(context), m_wait_semaphore(VK_NULL_HANDLE), m_signal_semaphore(VK_NULL_HANDLE), m_fence(VK_NULL_HANDLE)
{
	auto logical_device = m_context->m_logical_device;

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	if (vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &m_wait_semaphore) != VK_SUCCESS ||
	    vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &m_signal_semaphore) != VK_SUCCESS ||
	    vkCreateFence(logical_device, &fence_info, nullptr, &m_fence) != VK_SUCCESS)
	{
		LOGC("failed to create semaphores!");
	}
}

gfx::Fence::~Fence()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroySemaphore(logical_device, m_wait_semaphore, nullptr);
	vkDestroySemaphore(logical_device, m_signal_semaphore, nullptr);
	vkDestroyFence(logical_device, m_fence, nullptr);
}

void gfx::Fence::Wait()
{
	auto logical_device = m_context->m_logical_device;

	vkWaitForFences(logical_device, 1, &m_fence, VK_TRUE, UINT64_MAX);
	auto result = vkResetFences(logical_device, 1, &m_fence);
	if (result == VK_TIMEOUT)
	{
		LOGW("Fence timeout");
	}
}
