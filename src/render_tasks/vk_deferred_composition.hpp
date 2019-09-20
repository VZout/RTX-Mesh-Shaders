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
#include "../graphics/gfx_enums.hpp"

namespace tasks
{

	struct DeferredCompositionData
	{
		std::vector<gfx::GPUBuffer*> m_cbs;
		std::vector<std::vector<std::uint32_t>> m_cb_sets;
		std::uint32_t m_gbuffer_set;
		std::uint32_t m_uav_target_set;
		gfx::DescriptorHeap* m_gbuffer_heap;
	};

	namespace internal
	{

		inline void SetupDeferredCompositionTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
			auto context = rs.GetContext();
			auto root_sig = rs.GetCompoRootSignature();
			auto render_target = fg.GetRenderTarget(handle);
			auto deferred_main_rt = fg.GetPredecessorRenderTarget<DeferredMainData>();

			gfx::SamplerDesc gbuffer_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_POINT,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 3;
			data.m_gbuffer_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);
			data.m_gbuffer_set = data.m_gbuffer_heap->CreateSRVSetFromRT(deferred_main_rt, root_sig, 1, 0, false, gbuffer_sampler_desc);
			data.m_uav_target_set = data.m_gbuffer_heap->CreateUAVSetFromRT(render_target, 0, root_sig, 2, 0, gbuffer_sampler_desc);

			if (resize) return;

			auto desc_heap = rs.GetDescHeap();
			data.m_cb_sets.resize(gfx::settings::num_back_buffers);

			// Descriptors uniform
			data.m_cbs.resize(gfx::settings::num_back_buffers);
			for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
			{
				data.m_cbs[i] = new gfx::GPUBuffer(context, sizeof(cb::Basic), gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
				data.m_cbs[i]->Map();

				data.m_cb_sets[i].push_back(desc_heap->CreateSRVFromCB(data.m_cbs[i], root_sig, 0, i));
			}
		}

		inline void ExecuteDeferredCompositionTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto frame_idx = rs.GetFrameIdx();
			auto root_sig = rs.GetCompoRootSignature();
			auto pipeline = rs.GetCompoPipeline();
			auto desc_heap = rs.GetDescHeap();
			auto render_target = fg.GetRenderTarget(handle);

			glm::vec3 cam_pos = glm::vec3(0, 0, -2.5);

			cb::Basic basic_cb_data;
			data.m_cbs[frame_idx]->Update(&basic_cb_data, sizeof(cb::Basic));

			std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
			{
				{ desc_heap, data.m_cb_sets[frame_idx][0] },
				{ data.m_gbuffer_heap, data.m_gbuffer_set },
				{ data.m_gbuffer_heap, data.m_uav_target_set },
			};

			cmd_list->BindComputePipelineState(pipeline);
			cmd_list->BindComputeDescriptorHeap(root_sig, sets);
			cmd_list->Dispatch(render_target->GetWidth() / 16, render_target->GetHeight() / 16, 1);
		}

		inline void DestroyDeferredCompositionTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
			delete data.m_gbuffer_heap;

			if (resize) return;

			for (auto& cb : data.m_cbs)
			{
				delete cb;
			}
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
			.m_state_execute = VK_IMAGE_LAYOUT_GENERAL,
			.m_state_finished = std::nullopt,
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
		desc.m_type = fg::RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredCompositionData>(desc, L"Deferred Composition Task", FG_DEPS<DeferredMainData>());
	}

} /* tasks */
