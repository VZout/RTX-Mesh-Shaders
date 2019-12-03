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
	desc.m_num_descriptors = 400;
	desc.m_versions = 1;

	m_heap = new gfx::DescriptorHeap(context, desc);

	m_big_vertex_buffer = new gfx::GPUBuffer(m_context, std::nullopt, 1000000 * 100, gfx::enums::BufferUsageFlag::STORAGE_DST_VERTEX_BUFFER, VMA_MEMORY_USAGE_GPU_ONLY);
	m_big_index_buffer = new gfx::GPUBuffer(m_context, std::nullopt, 1000000 * 100, gfx::enums::BufferUsageFlag::STORAGE_DST_INDEX_BUFFER, VMA_MEMORY_USAGE_GPU_ONLY);

	auto& rs_reg = RootSignatureRegistry::Get();
	auto rs = rs_reg.Find(root_signatures::raytracing);
	m_big_vb_desc_set_id = m_heap->CreateSRVFromCB(m_big_vertex_buffer, rs, 4, 0, false);
	m_big_ib_desc_set_id = m_heap->CreateSRVFromCB(m_big_index_buffer, rs, 5, 0, false);
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

	delete m_big_index_buffer;
	delete m_big_vertex_buffer;
	destroy_func(m_meshlet_buffers);
	destroy_func(m_meshlet_vi_buffers);
	destroy_func(m_meshlet_fi_buffers);
	destroy_func(m_buffers_require_staging);

	delete m_heap;
}

ModelHandle::MeshOffsets gfx::VkModelPool::AllocateMesh(void* vertex_data, std::uint32_t num_vertices, std::uint32_t vertex_stride,
	void* index_data, std::uint32_t num_indices, std::uint32_t index_stride, void* meshlet_data, std::uint32_t num_meshlets)
{
	auto mb = new gfx::StagingBuffer(m_context, std::nullopt, std::nullopt, meshlet_data, num_meshlets, sizeof(MeshletDesc), gfx::enums::BufferUsageFlag::INDEX_BUFFER);

	auto vb_staging = new gfx::GPUBuffer(m_context, std::nullopt, vertex_data, num_vertices, vertex_stride, gfx::enums::BufferUsageFlag::TRANSFER_SRC, VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto ib_staging = new gfx::GPUBuffer(m_context, std::nullopt, index_data, num_indices, index_stride, gfx::enums::BufferUsageFlag::TRANSFER_SRC, VMA_MEMORY_USAGE_CPU_TO_GPU);
	m_vb_staging_buffers.push_back({ vb_staging, m_next_vb_offset });
	m_ib_staging_buffers.push_back({ ib_staging, m_next_ib_offset });

	auto& rs_reg = RootSignatureRegistry::Get();
	auto rs = rs_reg.Find(root_signatures::basic_mesh);

	auto descriptor_set_id = m_heap->CreateSRVFromCB(mb, rs, 6, 0, false);
	std::pair<std::uint64_t, std::uint64_t> vb_offset_size = { m_next_vb_offset, (std::uint64_t)vertex_stride * num_vertices };
	std::pair<std::uint64_t, std::uint64_t> ib_offset_size = { m_next_ib_offset, (std::uint64_t)index_stride * num_indices };
	auto vbi = m_heap->CreateSRVFromCB(m_big_vertex_buffer, rs, 4, 0, false, vb_offset_size);
	auto ibi = m_heap->CreateSRVFromCB(m_big_index_buffer, rs, 5, 0, false, ib_offset_size);

	m_meshlet_buffers.push_back(mb);
	m_meshlet_desc_infos.push_back({ descriptor_set_id, num_meshlets });
	m_mesh_shading_buffer_descriptor_sets.push_back({ vbi, ibi });

	m_buffers_require_staging.push_back(mb);

	ModelHandle::MeshOffsets offsets
	{
		.m_vb = m_next_vb_offset,
		.m_ib = m_next_ib_offset
	};


	const auto storage_buffer_allignment = m_context->GetPhysicalDeviceProperties().properties.limits.minStorageBufferOffsetAlignment;
	auto x = SizeAlignTwoPower(vertex_stride, storage_buffer_allignment);
	m_next_vb_offset += SizeAlignTwoPower(vertex_stride * num_vertices, 1);
	m_next_ib_offset += SizeAlignTwoPower(index_stride * num_indices, storage_buffer_allignment);

	return offsets;
}

void gfx::VkModelPool::AllocateMeshShadingBuffers(std::vector<std::uint32_t> vertex_indices, std::vector<std::uint8_t> flat_indices)
{
	auto vi_buffer = new gfx::StagingBuffer(m_context, std::nullopt, std::nullopt, vertex_indices.data(), vertex_indices.size(), sizeof(std::uint32_t), gfx::enums::BufferUsageFlag::INDEX_BUFFER);
	auto fi_buffer = new gfx::StagingBuffer(m_context, std::nullopt, std::nullopt, flat_indices.data(), flat_indices.size(), sizeof(std::uint8_t), gfx::enums::BufferUsageFlag::INDEX_BUFFER);

	m_meshlet_vi_buffers.push_back(vi_buffer);
	m_meshlet_fi_buffers.push_back(fi_buffer);

	auto& rs_reg = RootSignatureRegistry::Get();
	auto rs = rs_reg.Find(root_signatures::basic_mesh);
	auto vi_desc = m_heap->CreateSRVFromCB(vi_buffer, rs, 7, 0, false);
	auto fi_desc = m_heap->CreateSRVFromCB(fi_buffer, rs, 5, 0, false);

	m_mesh_shading_index_buffer_descriptor_sets.push_back({ vi_desc, fi_desc });

	m_buffers_require_staging.push_back(vi_buffer);
	m_buffers_require_staging.push_back(fi_buffer);
}

void gfx::VkModelPool::Stage(CommandList* command_list)
{
	for (auto& buffer : m_buffers_require_staging)
	{
		command_list->StageBuffer(buffer);
	}
	m_buffers_require_staging.clear();

	for (auto& pair_buffer_offset : m_vb_staging_buffers)
	{
		command_list->StageBuffer(m_big_vertex_buffer, pair_buffer_offset.first, pair_buffer_offset.second);
		m_buffers_require_cleanup.push_back(pair_buffer_offset.first);
	}
	m_vb_staging_buffers.clear();

	for (auto& pair_buffer_offset : m_ib_staging_buffers)
	{
		command_list->StageBuffer(m_big_index_buffer, pair_buffer_offset.first, pair_buffer_offset.second);
		m_buffers_require_cleanup.push_back(pair_buffer_offset.first);
	}
	m_ib_staging_buffers.clear();
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

	for (auto& buffer : m_buffers_require_cleanup)
	{
		delete buffer;
	}
	m_buffers_require_cleanup.clear();

	free_func(m_meshlet_buffers);
	free_func(m_meshlet_vi_buffers);
	free_func(m_meshlet_fi_buffers);
}

gfx::DescriptorHeap* gfx::VkModelPool::GetDescriptorHeap()
{
	return m_heap;
}