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
#include "../root_signature_registry.hpp"
#include "../pipeline_registry.hpp"
#include "../graphics/vk_constant_buffer_pool.hpp"

namespace tasks
{

	struct DeferredMainData
	{
		std::vector<std::vector<std::uint32_t>> m_material_sets;

		gfx::RootSignature* m_root_sig;
	};

	namespace internal
	{

		inline void SetupDeferredMainTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<DeferredMainData>(handle);
			auto context = rs.GetContext();
			auto desc_heap = rs.GetDescHeap();
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::basic);

			data.m_material_sets.resize(gfx::settings::num_back_buffers);
		}

		inline void ExecuteDeferredMainTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredMainData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto model_pool = static_cast<gfx::VkModelPool*>(rs.GetModelPool());
			auto pipeline = PipelineRegistry::SFind(pipelines::basic);
			auto material_pool = static_cast<gfx::VkMaterialPool*>(rs.GetMaterialPool());

			auto per_obj_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetPOConstantBufferPool());
			auto camera_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetCameraConstantBufferPool());

			cmd_list->BindPipelineState(pipeline);

			auto mesh_node_handles = sg.GetMeshNodeHandles();
			auto camera_handle = sg.m_camera_cb_handles[0].m_value;
			for (auto const & handle : mesh_node_handles)
			{
				auto node = sg.GetNode(handle);
				auto model_handle = sg.m_model_handles[node.m_mesh_component].m_value;
				auto cb_handle = sg.m_transform_cb_handles[node.m_mesh_component].m_value;

				for (std::size_t i = 0; i < model_handle.m_mesh_handles.size(); i++)
				{
					auto mesh_handle = model_handle.m_mesh_handles[i];

					std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
					{
						{ camera_pool->GetDescriptorHeap(), camera_handle.m_cb_set_id }, // TODO: Shitty naming of set_id. just use a vector in the handle instead probably.
						{ per_obj_pool->GetDescriptorHeap(), cb_handle.m_cb_set_id }, // TODO: Shitty naming of set_id. just use a vector in the handle instead probably.
						{ material_pool->GetDescriptorHeap(), mesh_handle.m_material_handle->m_material_set_id }
					};

					cmd_list->BindDescriptorHeap(data.m_root_sig, sets);
					cmd_list->BindVertexBuffer(model_pool->m_vertex_buffers[mesh_handle.m_id]);
					cmd_list->BindIndexBuffer(model_pool->m_index_buffers[mesh_handle.m_id]);
					cmd_list->DrawIndexed(mesh_handle.m_num_indices, 1);
				}
			}
		}

		inline void DestroyDeferredMainTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{

		}

	} /* internal */

	inline void AddDeferredMainTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_dsv_format = VK_FORMAT_D32_SFLOAT,
			.m_rtv_formats = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
			.m_state_execute = std::nullopt,
			.m_state_finished = VK_IMAGE_LAYOUT_GENERAL,
			.m_clear = true,
			.m_clear_depth = true,
			.m_allow_direct_access = true
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupDeferredMainTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteDeferredMainTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyDeferredMainTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredMainData>(desc, L"Deferred Rasterization Task");
	}

} /* tasks */
