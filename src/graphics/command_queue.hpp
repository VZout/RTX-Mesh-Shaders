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