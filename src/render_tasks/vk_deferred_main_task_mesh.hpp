/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "../application.hpp"
#include "../buffer_definitions.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../renderer.hpp"
#include "../model_pool.hpp"
#include "../texture_pool.hpp"
#include "../vertex.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../graphics/vk_model_pool.hpp"
#include "../graphics/descriptor_heap.hpp"
#include "../graphics/vk_material_pool.hpp"
#include "../engine_registry.hpp"
#include "../graphics/vk_constant_buffer_pool.hpp"

namespace tasks
{

	struct DeferredMainMeshData
	{
		std::vector<std::vector<std::uint32_t>> m_material_sets;
		
		gfx::PipelineState* m_pipeline;
		gfx::RootSignature* m_root_sig;
	};

	namespace internal
	{

		inline void SetupDeferredMainMeshTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<DeferredMainMeshData>(handle);
			auto context = rs.GetContext();
			auto desc_heap = rs.GetDescHeap();
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::basic_mesh);
			data.m_pipeline = PipelineRegistry::SFind(pipelines::basic_mesh);
			data.m_material_sets.resize(gfx::settings::num_back_buffers);
		}

		inline void ExecuteDeferredMainMeshTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredMainMeshData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto model_pool = static_cast<gfx::VkModelPool*>(rs.GetModelPool());
			auto material_pool = static_cast<gfx::VkMaterialPool*>(rs.GetMaterialPool());
			auto per_obj_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetPOConstantBufferPool());
			auto camera_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetCameraConstantBufferPool());

			cmd_list->BindPipelineState(data.m_pipeline);

			auto mesh_node_handles = sg.GetMeshNodeHandles();
			auto camera_handle = sg.m_camera_cb_handles[0].m_value;
			/*for (auto const & handle : mesh_node_handles)
			{
				auto node = sg.GetNode(handle);
				auto model_handle = sg.m_model_handles[node.m_mesh_component].m_value;
				auto cb_handle = sg.m_transform_cb_handles[node.m_mesh_component].m_value
				auto mat_vec = sg.m_model_material_handles[node.m_mesh_component].m_value;

				for (std::size_t i = 0; i < model_handle.m_mesh_handles.size(); i++)
				{
					auto mesh_handle = model_handle.m_mesh_handles[i];
					auto meshlets_info = model_pool->m_meshlet_desc_infos[mesh_handle.m_id];
					auto vb_ib_pair = model_pool->m_mesh_shading_buffer_descriptor_sets[mesh_handle.m_id];

					std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
					{
						{ camera_pool->GetDescriptorHeap(), camera_handle.m_cb_set_id }, // TODO: Shitty naming of set_id. just use a vector in the handle instead probably.
						{ per_obj_pool->GetDescriptorHeap(), cb_handle.m_cb_set_id }, // TODO: Shitty naming of set_id. just use a vector in the handle instead probably.
						{ material_pool->GetDescriptorHeap(), material_pool->GetDescriptorSetID(mat_vec[i]) },
						{ material_pool->GetDescriptorHeap(), material_pool->GetCBDescriptorSetID(mat_vec[i]) },
						{ model_pool->GetDescriptorHeap(), vb_ib_pair.first },
						{ model_pool->GetDescriptorHeap(), vb_ib_pair.second },
						{ model_pool->GetDescriptorHeap(), meshlets_info.first }
					};

					cmd_list->BindDescriptorHeap(data.m_root_sig, sets);
					cmd_list->DrawMesh(meshlets_info.second, 0);
				}

			}*/

			for (auto const& batch : sg.GetRenderBatches())
			{
				auto model_handle = batch.m_model_handle;
				auto cb_handle = batch.m_big_cb;
				auto const& mat_vec = batch.m_material_handles;

				for (std::size_t i = 0; i < model_handle.m_mesh_handles.size(); i++)
				{
					auto mesh_handle = model_handle.m_mesh_handles[i];
					auto meshlets_info = model_pool->m_meshlet_desc_infos[mesh_handle.m_id];
					auto vb_ib_pair = model_pool->m_mesh_shading_buffer_descriptor_sets[mesh_handle.m_id];

					std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
					{
						{ camera_pool->GetDescriptorHeap(), camera_handle.m_cb_set_id }, // TODO: Shitty naming of set_id. just use a vector in the handle instead probably.
						{ per_obj_pool->GetDescriptorHeap(), cb_handle.m_cb_set_id }, // TODO: Shitty naming of set_id. just use a vector in the handle instead probably.
						{ material_pool->GetDescriptorHeap(), material_pool->GetDescriptorSetID(mat_vec[i]) },
						{ material_pool->GetDescriptorHeap(), material_pool->GetCBDescriptorSetID(mat_vec[i]) },
						{ model_pool->GetDescriptorHeap(), vb_ib_pair.first },
						{ model_pool->GetDescriptorHeap(), vb_ib_pair.second },
						{ model_pool->GetDescriptorHeap(), meshlets_info.first }
					};

					cmd_list->BindDescriptorHeap(data.m_root_sig, sets);
					cmd_list->DrawMesh(meshlets_info.second, 0);
				}
			}
		}

		inline void DestroyDeferredMainMeshTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{

		}

	} /* internal */

	inline void AddDeferredMainMeshTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_dsv_format = VK_FORMAT_D32_SFLOAT,
			.m_rtv_formats = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
			.m_state_execute = std::nullopt,
			.m_state_finished = VK_IMAGE_LAYOUT_GENERAL,
			.m_clear = true,
			.m_clear_depth = true,
			.m_allow_direct_access = true
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupDeferredMainMeshTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteDeferredMainMeshTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyDeferredMainMeshTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredMainMeshData>(desc, L"Deferred Rasterization Task");
	}

} /* tasks */
