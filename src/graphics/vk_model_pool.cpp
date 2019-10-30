/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "vk_model_pool.hpp"

#include "context.hpp"
#include "command_list.hpp"
#include "gpu_buffers.hpp"
#include "descriptor_heap.hpp"
#include "../engine_registry.hpp"

gfx::VkModelPool::VkModelPool(Context* context)
		: ModelPool(), m_context(context)
{
	gfx::DescriptorHeap::Desc desc = {};
	desc.m_num_descriptors = 100;
	desc.m_versions = 1;

	m_heap = new gfx::DescriptorHeap(context, desc);
}

gfx::VkModelPool::~VkModelPool()
{
	auto destroy_func = [](auto& buffer_list)
	{
		for (auto& buffer : buffer_list)
		{
			delete buffer;
		}
		buffer_list.clear();
	};

	destroy_func(m_vertex_buffers);
	destroy_func(m_index_buffers);
	destroy_func(m_meshlet_buffers);
	destroy_func(m_buffers_require_staging);

	delete m_heap;
}

void gfx::VkModelPool::AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
	void* index_data, std::uint32_t num_indices, std::uint32_t index_stride, void* meshlet_data, std::uint32_t num_meshlets)
{
	auto vb = new gfx::StagingBuffer(m_context, std::nullopt, std::nullopt, vertex_data, num_vertices, vertex_stride, gfx::enums::BufferUsageFlag::VERTEX_BUFFER);
	auto ib = new gfx::StagingBuffer(m_context, std::nullopt, std::nullopt, index_data, num_indices, index_stride, gfx::enums::BufferUsageFlag::INDEX_BUFFER);
	auto mb = new gfx::StagingBuffer(m_context, std::nullopt, std::nullopt, meshlet_data, num_meshlets, sizeof(MeshletDesc), gfx::enums::BufferUsageFlag::INDEX_BUFFER);

	auto& rs_reg = RootSignatureRegistry::Get();
	auto rs = rs_reg.Find(root_signatures::basic_mesh);
	auto descriptor_set_id = m_heap->CreateSRVFromCB(mb, rs, 6, 0, false);

	auto vbi = m_heap->CreateSRVFromCB(vb, rs, 4, 0, false);
	auto ibi = m_heap->CreateSRVFromCB(ib, rs, 5, 0, false);

	m_vertex_buffers.push_back(vb);
	m_index_buffers.push_back(ib);
	m_meshlet_buffers.push_back(mb);
	m_meshlet_desc_infos.push_back({ descriptor_set_id, num_meshlets });
	m_mesh_shading_buffer_descriptor_sets.push_back({ vbi, ibi });

	m_buffers_require_staging.push_back(vb);
	m_buffers_require_staging.push_back(ib);
	m_buffers_require_staging.push_back(mb);
}

void gfx::VkModelPool::Stage(CommandList* command_list)
{
	for (auto& buffer : m_buffers_require_staging)
	{
		command_list->StageBuffer(buffer);
	}
	m_buffers_require_staging.clear();
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
	free_func(m_meshlet_buffers);
}

gfx::DescriptorHeap* gfx::VkModelPool::GetDescriptorHeap()
{
	return m_heap;
}