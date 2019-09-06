/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "vulkan/vulkan.h"

#include <vector>
#include <cstdint>

#include "command_queue.hpp"

namespace gfx
{
	class Context;
	class RenderTarget;
	class PipelineState;

	class CommandList
	{
		friend class RenderWindow;
		friend class CommandQueue;
	public:
		explicit CommandList(CommandQueue* queue);
		~CommandList();

		void Begin(std::uint32_t frame_idx);
		void Close(std::uint32_t frame_idx);
		void BindRenderTargetVersioned(RenderTarget* render_target, std::uint32_t frame_idx);
		void BindPipelineState(PipelineState* pipeline, std::uint32_t frame_idx);
		void DrawInstanced(std::uint32_t frame_idx, std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex = 0, std::uint32_t first_instance = 0);

	private:
		VkCommandPool m_cmd_pool;
		VkCommandPoolCreateInfo m_cmd_pool_create_info;
		std::vector<VkCommandBuffer> m_cmd_buffers;

		CommandQueue* m_queue;
		Context* m_context;
	};

} /* gfx */
