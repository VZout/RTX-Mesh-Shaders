/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "vk_model_pool.hpp"

#include "context.hpp"
#include "command_list.hpp"
#include "gpu_buffers.hpp"

gfx::VkModelPool::VkModelPool(Context* context)
		: ModelPool(), m_context(context)
{

}

gfx::VkModelPool::~VkModelPool()
{
	auto destroy_func = [](auto buffer_list)
	{
		for (auto& buffer : buffer_list)
		{
			delete buffer;
		}
		buffer_list.clear();
	};

	destroy_func(m_vertex_buffers);
	destroy_func(m_index_buffers);
}

void gfx::VkModelPool::AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
		void* index_data, std::uint32_t num_indices, std::uint32_t index_stride)
{
	auto vb = new gfx::StagingBuffer(m_context, vertex_data, num_vertices, vertex_stride, gfx::enums::BufferUsageFlag::VERTEX_BUFFER);
	auto ib = new gfx::StagingBuffer(m_context, index_data, num_indices, index_stride, gfx::enums::BufferUsageFlag::INDEX_BUFFER);

	m_vertex_buffers.push_back(vb);
	m_index_buffers.push_back(ib);
}

void gfx::VkModelPool::Stage(CommandList* command_list, std::uint32_t frame_idx)
{
	auto stage_func = [command_list, frame_idx](auto buffer_list)
	{
		for (auto& buffer : buffer_list)
		{
			command_list->StageBuffer(buffer, frame_idx);
		}
	};

	stage_func(m_vertex_buffers);
	stage_func(m_index_buffers);
}

void gfx::VkModelPool::PostStage()
{
	auto free_func = [](auto buffer_list)
	{
		for (auto& buffer : buffer_list)
		{
			buffer->FreeStagingResources();
		}
	};

	free_func(m_vertex_buffers);
	free_func(m_index_buffers);
}