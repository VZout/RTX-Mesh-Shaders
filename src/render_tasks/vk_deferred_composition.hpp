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
#include "vk_deferred_main_task.hpp"

namespace tasks
{

	struct DeferredCompositionData
	{
		std::vector<gfx::GPUBuffer*> m_cbs;
		std::vector<std::vector<std::uint32_t>> m_cb_sets;
		std::vector<std::uint32_t> m_gbuffer_sets;
		gfx::DescriptorHeap* m_gbuffer_heap;
	};

	namespace internal
	{

		inline void SetupDeferredCompositionTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<DeferredCompositionData>(handle);
			auto context = rs.GetContext();
			auto desc_heap = rs.GetDescHeap();
			auto root_sig = rs.GetCompoRootSignature();

			auto deferred_main_rt = fg.GetPredecessorRenderTarget<DeferredMainData>();

			data.m_cb_sets.resize(gfx::settings::num_back_buffers);

			// Descriptors uniform
			data.m_cbs.resize(gfx::settings::num_back_buffers);
			for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
			{
				data.m_cbs[i] = new gfx::GPUBuffer(context, sizeof(cb::Basic), gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
				data.m_cbs[i]->Map();

				data.m_cb_sets[i].push_back(desc_heap->CreateSRVFromCB(data.m_cbs[i], root_sig, 0, i));
			}

			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 3;
			data.m_gbuffer_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);
			data.m_gbuffer_sets.push_back(data.m_gbuffer_heap->CreateSRVSetFromRT(deferred_main_rt, root_sig, 1, 0, false));
		}

		inline void ExecuteDeferredCompositionTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto frame_idx = rs.GetFrameIdx();
			auto root_sig = rs.GetCompoRootSignature();
			auto pipeline = rs.GetCompoPipeline();
			auto desc_heap = rs.GetDescHeap();
			auto render_window = rs.GetRenderWindow();
			auto deferred_main_rt = fg.GetPredecessorRenderTarget<DeferredMainData>();

			glm::vec3 cam_pos = glm::vec3(0, 0, -2.5);

			cb::Basic basic_cb_data;
			basic_cb_data.m_model = sg.m_models[0];
			basic_cb_data.m_view = glm::lookAt(cam_pos, cam_pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			basic_cb_data.m_proj = glm::perspective(glm::radians(45.0f), (float) render_window->GetWidth() / (float) render_window->GetHeight(), 0.01f, 1000.0f);
			basic_cb_data.m_proj[1][1] *= -1;
			data.m_cbs[frame_idx]->Update(&basic_cb_data, sizeof(cb::Basic));

			std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
			{
				{ desc_heap, data.m_cb_sets[frame_idx][0] },
				{ data.m_gbuffer_heap, data.m_gbuffer_sets[0] },
			};

			//cmd_list->TransitionRenderTarget(deferred_main_rt, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, frame_idx);

			cmd_list->BindPipelineState(pipeline, frame_idx);
			cmd_list->BindDescriptorHeap(root_sig, sets, frame_idx);
			cmd_list->Draw(frame_idx, 4, 1);

			//cmd_list->TransitionRenderTarget(deferred_main_rt, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, frame_idx);
		}

		inline void DestroyDeferredCompositionTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<DeferredCompositionData>(handle);

			for (auto& cb : data.m_cbs)
			{
				delete cb;
			}

			delete data.m_gbuffer_heap;
		}

	} /* internal */

	inline void AddDeferredCompositionTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_dsv_format = VK_FORMAT_UNDEFINED,
			.m_rtv_formats = { VK_FORMAT_B8G8R8A8_UNORM },
			.m_state_execute = std::nullopt,
			.m_state_finished = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.m_clear = false,
			.m_clear_depth = false
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupDeferredCompositionTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteDeferredCompositionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyDeferredCompositionTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = false;

		fg.AddTask<DeferredCompositionData>(desc, L"Deferred Composition Task", FG_DEPS<DeferredMainData>());
	}

} /* tasks */
