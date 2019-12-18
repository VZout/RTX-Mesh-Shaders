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
	class GPUBuffer;
	class DescriptorHeap;

	class VkModelPool : public ModelPool
	{
	public:
		explicit VkModelPool(Context* context);
		~VkModelPool() final;

		ModelHandle::MeshOffsets AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
				void* index_data, std::uint32_t num_indices, std::uint32_t index_stride, void* meshlet_data, std::uint32_t num_meshlets) final;


		void AllocateMeshShadingBuffers(std::vector<std::uint32_t> vertex_indices, std::vector<std::uint8_t> flat_indices) final;

		void Stage(CommandList* command_list) final;
		void PostStage() final;

		gfx::DescriptorHeap* GetDescriptorHeap();

	protected:
		Context* m_context;

	public:
		GPUBuffer* m_big_vertex_buffer;
		std::vector<std::pair<GPUBuffer*, std::uint64_t>> m_vb_staging_buffers;
		std::uint64_t m_next_vb_offset = 0;
		std::uint32_t m_big_vb_desc_set_id;

		GPUBuffer* m_big_index_buffer;
		std::vector<std::pair<GPUBuffer*, std::uint64_t>> m_ib_staging_buffers;
		std::uint64_t m_next_ib_offset = 0;
		std::uint32_t m_big_ib_desc_set_id;

		std::vector<GPUBuffer*> m_buffers_require_cleanup;
		std::vector<StagingBuffer*> m_buffers_require_staging;
		std::vector<StagingBuffer*> m_meshlet_buffers;
		std::vector<StagingBuffer*> m_meshlet_vi_buffers;
		std::vector<StagingBuffer*> m_meshlet_fi_buffers;

		std::vector<std::pair<std::uint32_t, std::uint32_t>> m_meshlet_desc_infos;
		std::vector<std::pair<std::uint32_t, std::uint32_t>> m_mesh_shading_buffer_descriptor_sets;
		std::vector<std::pair<std::uint32_t, std::uint32_t>> m_mesh_shading_index_buffer_descriptor_sets;

		gfx::DescriptorHeap* m_heap;
	};

} /* gfx */