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
	class DescriptorHeap;

	class VkModelPool : public ModelPool
	{
	public:
		explicit VkModelPool(Context* context);
		~VkModelPool() final;

		void AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
				void* index_data, std::uint32_t num_indices, std::uint32_t index_stride, void* meshlet_data, std::uint32_t num_meshlets) final;

		void Stage(CommandList* command_list) final;
		void PostStage() final;

		gfx::DescriptorHeap* GetDescriptorHeap();

	protected:
		Context* m_context;

	public:
		std::vector<StagingBuffer*> m_vertex_buffers;
		std::vector<StagingBuffer*> m_index_buffers;
		std::vector<StagingBuffer*> m_meshlet_buffers;
		std::vector<std::pair<std::uint32_t, std::uint32_t>> m_meshlet_desc_infos;
		gfx::DescriptorHeap* m_heap;
	};

} /* gfx */