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

namespace tasks
{

	struct DeferredMainData
	{
		std::vector<gfx::GPUBuffer*> m_cbs;
		std::vector<std::vector<std::uint32_t>> m_cb_sets;
		std::vector<std::vector<std::uint32_t>> m_material_sets;
	};

	namespace internal
	{

		inline void SetupDeferredMainTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<DeferredMainData>(handle);
			auto context = rs.GetContext();
			auto desc_heap = rs.GetDescHeap();
			auto root_sig = rs.GetRootSignature();

			data.m_cb_sets.resize(gfx::settings::num_back_buffers);
			data.m_material_sets.resize(gfx::settings::num_back_buffers);

			// Descriptors uniform
			data.m_cbs.resize(gfx::settings::num_back_buffers);
			for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
			{
				data.m_cbs[i] = new gfx::GPUBuffer(context, sizeof(cb::Basic), gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
				data.m_cbs[i]->Map();

				data.m_cb_sets[i].push_back(desc_heap->CreateSRVFromCB(data.m_cbs[i], root_sig, 0, i));
			}
		}

		inline void ExecuteDeferredMainTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredMainData>(handle);
			auto render_window = fg.GetRenderTarget<gfx::RenderWindow>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto frame_idx = rs.GetFrameIdx();
			auto model_pool = static_cast<gfx::VkModelPool*>(rs.GetModelPool());
			auto root_sig = rs.GetRootSignature();
			auto pipeline = rs.GetPipeline();
			auto desc_heap = rs.GetDescHeap();
			auto material_pool = static_cast<gfx::VkMaterialPool*>(rs.GetMaterialPool());

			glm::vec3 cam_pos = glm::vec3(0, 0, -2.5);

			cb::Basic basic_cb_data;
			basic_cb_data.m_model = sg.m_models[0];
			basic_cb_data.m_view = glm::lookAt(cam_pos, cam_pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			basic_cb_data.m_proj = glm::perspective(glm::radians(45.0f), (float) render_window->GetWidth() / (float) render_window->GetHeight(), 0.01f, 1000.0f);
			basic_cb_data.m_proj[1][1] *= -1;
			data.m_cbs[frame_idx]->Update(&basic_cb_data, sizeof(cb::Basic));

			cmd_list->BindPipelineState(pipeline, frame_idx);
			auto model_handle = sg.m_model_handles[0].m_value;
			for (std::size_t i = 0; i < model_handle.m_mesh_handles.size(); i++)
			{
				auto mesh_handle = model_handle.m_mesh_handles[i];

				std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
				{
					{ desc_heap, data.m_cb_sets[frame_idx][0] },
					{ material_pool->GetDescriptorHeap(), mesh_handle.m_material_handle->m_material_set_id }
				};

				cmd_list->BindDescriptorHeap(root_sig, sets, frame_idx);
				cmd_list->BindVertexBuffer(model_pool->m_vertex_buffers[i], frame_idx);
				cmd_list->BindIndexBuffer(model_pool->m_index_buffers[i], frame_idx);
				cmd_list->DrawIndexed(frame_idx, mesh_handle.m_num_indices, 1);
			}
		}

		inline void DestroyDeferredMainTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DeferredMainData>(handle);

			if (resize)
			{

			}
			else
			{
				for (auto& cb : data.m_cbs)
				{
					delete cb;
				}
			}
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
			.m_rtv_formats = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
			.m_state_execute = std::nullopt,
			.m_state_finished = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.m_clear = true,
			.m_clear_depth = true
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
		desc.m_allow_multithreading = false;

		fg.AddTask<DeferredMainData>(desc, L"Deferred Rasterization Task");
	}

} /* tasks */
