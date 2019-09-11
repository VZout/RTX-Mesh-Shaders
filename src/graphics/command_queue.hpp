/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "vulkan/vulkan.h"
#include "context.hpp"


#include <vector>
#include <cstdint>

class Renderer;
class ImGuiImpl;

namespace gfx
{
	class Context;
	class Fence;
	class CommandList;

	enum class CommandQueueType
	{
		DIRECT,
		COPY,
		COMPUTE
	};

	class CommandQueue
	{
		friend class RenderWindow;
		friend class CommandList;
		friend class ::ImGuiImpl;
	public:
		CommandQueue(Context* context, CommandQueueType queue_type);
		~CommandQueue() = default;

		void Execute(std::vector<CommandList*> cmd_lists, Fence* fence, std::uint32_t frame_idx);
		void Wait();

	private:
		Context* m_context;

		CommandQueueType m_type;
		VkQueue m_queue;
	};

} /* gfx */
