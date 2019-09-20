/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "../model_pool.hpp"

namespace gfx
{

	class Context;
	class StagingBuffer;

	class VkModelPool : public ModelPool
	{
	public:
		explicit VkModelPool(Context* context);
		~VkModelPool() final;

		void AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
				void* index_data, std::uint32_t num_indices, std::uint32_t index_stride) final;

		void Stage(CommandList* command_list) final;
		void PostStage() final;

	protected:
		Context* m_context;

	public:
		std::vector<StagingBuffer*> m_vertex_buffers;
		std::vector<StagingBuffer*> m_index_buffers;
	};

} /* gfx */