/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>

namespace gfx
{

	class Context;

	class Fence
	{
		friend class RenderWindow;
		friend class CommandQueue;
	public:
		Fence(Context* context);
		~Fence();

		void Wait();
	
	private:
		VkSemaphore m_wait_semaphore;
		VkSemaphore m_signal_semaphore;
		VkFence m_fence;

		Context* m_context;
	};

} /* gfx */

