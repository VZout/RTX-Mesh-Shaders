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
#include "../engine_registry.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../graphics/vk_model_pool.hpp"
#include "../graphics/descriptor_heap.hpp"
#include "../graphics/vk_material_pool.hpp"
#include "../graphics/gfx_enums.hpp"
#include "vk_raytracing_task.hpp"

namespace tasks
{

	struct TAAData
	{
		std::uint32_t m_input_set;
		std::uint32_t m_uav_target_set;
		gfx::DescriptorHeap* m_gbuffer_heap;

		gfx::RootSignature* m_root_sig;
	};

	namespace internal
	{

		template<typename T>
		inline void SetupTAATask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool)
		{
			auto& data = fg.GetData<TAAData>(handle);
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::taa);
			auto render_target = fg.GetRenderTarget(handle);
			auto predecessor_rt = fg.GetPredecessorRenderTarget<T>();

			gfx::SamplerDesc input_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_LINEAR,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 3;
			data.m_gbuffer_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);
			data.m_input_set = data.m_gbuffer_heap->CreateSRVSetFromRT(predecessor_rt, data.m_root_sig, 0, 0, false, std::nullopt);
			data.m_uav_target_set = data.m_gbuffer_heap->CreateUAVSetFromRT(render_target, 0, data.m_root_sig, 1, 0, input_sampler_desc);
		}

		inline void ExecuteTAATask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<TAAData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto pipeline = PipelineRegistry::SFind(pipelines::taa);
			auto render_target = fg.GetRenderTarget(handle);

			auto rt_data = fg.GetPredecessorData<RaytracingData>();

			cb::Basic basic_cb_data;
			std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
			{
				{ data.m_gbuffer_heap, data.m_input_set },
				{ data.m_gbuffer_heap, data.m_uav_target_set },
			};

			auto frame_number = rt_data.frame_number - 1;

			cmd_list->BindPipelineState(pipeline);
			cmd_list->BindDescriptorHeap(data.m_root_sig, sets);
			cmd_list->BindComputePushConstants(data.m_root_sig, &frame_number, sizeof(std::uint32_t));
			cmd_list->Dispatch(render_target->GetWidth() / 16, render_target->GetHeight() / 16, 1);
		}

		inline void DestroyTAATask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool)
		{
			auto& data = fg.GetData<TAAData>(handle);
			delete data.m_gbuffer_heap;
		}

	} /* internal */

	template<typename T>
	inline void AddTemporalAntiAliasingTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = std::nullopt,
			.m_height = std::nullopt, 
			.m_dsv_format = VK_FORMAT_UNDEFINED,
			.m_rtv_formats = { VK_FORMAT_R32G32B32A32_SFLOAT },
			.m_state_execute = VK_IMAGE_LAYOUT_GENERAL,
			.m_state_finished = std::nullopt,
			.m_clear = false,
			.m_clear_depth = false
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupTAATask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteTAATask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyTAATask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<TAAData>(desc, "Temporal Anti Aliasing Task");
	}

} /* tasks */
