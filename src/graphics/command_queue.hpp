/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "vulkan/vulkan.h"

namespace gfx
{
	class Context;

	enum class CommandQueueType
	{
		DIRECT,
		COPY,
		COMPUTE
	};

	class CommandQueue
	{
	public:
		CommandQueue(Context* context, CommandQueueType queue_type);
		~CommandQueue() = default;

	private:
		VkQueue m_queue;
	};

} /* gfx */
