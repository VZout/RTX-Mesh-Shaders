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
#include "../graphics/shader_table.hpp"
#include "vk_build_acceleration_structures_task.hpp"

namespace tasks
{

	struct RaytracingData
	{
		std::uint32_t m_tlas_set;
		std::uint32_t m_uav_target_set;
		gfx::DescriptorHeap* m_gbuffer_heap;

		gfx::PipelineState* m_pipeline;
		gfx::RootSignature* m_root_sig;

		gfx::ShaderTable* m_raygen_shader_table;
		gfx::ShaderTable* m_miss_shader_table;
		gfx::ShaderTable* m_hitgroup_shader_table;

		bool m_first_execute = true;
	};

	namespace internal
	{

		inline void SetupRaytracingTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<RaytracingData>(handle);
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::raytracing);
			data.m_pipeline = RTPipelineRegistry::SFind(pipelines::raytracing);
			auto render_target = fg.GetRenderTarget(handle);

			gfx::SamplerDesc input_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_LINEAR,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 2;
			data.m_gbuffer_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);
			data.m_uav_target_set = data.m_gbuffer_heap->CreateUAVSetFromRT(render_target, 0, data.m_root_sig, 1, 0, input_sampler_desc);

			data.m_first_execute = true;

			if (resize) return;

			data.m_raygen_shader_table = new gfx::ShaderTable(rs.GetContext(), 3);
			data.m_raygen_shader_table->AddShaderRecord(data.m_pipeline, 0, 3);

			/*data.m_miss_shader_table = new gfx::ShaderTable(rs.GetContext(), 1);
			data.m_miss_shader_table->AddShaderRecord(data.m_pipeline, 1, 1);

			data.m_hitgroup_shader_table = new gfx::ShaderTable(rs.GetContext(), 1);
			data.m_hitgroup_shader_table->AddShaderRecord(data.m_pipeline, 2, 1);*/
		}

		inline void ExecuteRaytracingTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<RaytracingData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto render_target = fg.GetRenderTarget(handle);

			auto camera_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetInverseCameraConstantBufferPool());
			auto camera_handle = sg.m_inverse_camera_cb_handles[0].m_value;

			if (data.m_first_execute)
			{
				auto as_build_data = fg.GetPredecessorData<BuildASData>();
				data.m_tlas_set = data.m_gbuffer_heap->CreateSRVFromAS(as_build_data.m_tlas, data.m_root_sig, 0, 0);
				data.m_first_execute = false;
			}

			cb::Basic basic_cb_data;
			std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
			{
				{ data.m_gbuffer_heap, data.m_tlas_set },
				{ data.m_gbuffer_heap, data.m_uav_target_set },
				{ camera_pool->GetDescriptorHeap(), camera_handle.m_cb_set_id },
			};

			cmd_list->BindPipelineState(data.m_pipeline);
			cmd_list->BindDescriptorHeap(data.m_root_sig, sets);
			cmd_list->DispatchRays(data.m_raygen_shader_table, data.m_raygen_shader_table, data.m_raygen_shader_table, render_target->GetWidth(), render_target->GetHeight(), 1);
		}

		inline void DestroyRaytracingTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<RaytracingData>(handle);

			delete data.m_gbuffer_heap;

			if (resize) return;

			delete data.m_raygen_shader_table;
			delete data.m_miss_shader_table;
			delete data.m_hitgroup_shader_table;
		}

	} /* internal */

	inline void AddRaytracingTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_dsv_format = VK_FORMAT_UNDEFINED,
			.m_rtv_formats = { VK_FORMAT_B8G8R8A8_UNORM },
			.m_state_execute = VK_IMAGE_LAYOUT_GENERAL,
			.m_state_finished = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.m_clear = false,
			.m_clear_depth = false
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupRaytracingTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteRaytracingTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyRaytracingTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<RaytracingData>(desc, L"Raytracing Task", FG_DEPS<BuildASData>());
	}

} /* tasks */
