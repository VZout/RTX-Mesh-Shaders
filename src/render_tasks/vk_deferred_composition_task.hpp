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
#include "vk_generate_cubemap.hpp"
#include "vk_generate_irradiance_map.hpp"
#include "vk_generate_environment_map.hpp"
#include "vk_generate_brdf_lut.hpp"
#include "../graphics/gfx_enums.hpp"

namespace tasks
{

	struct DeferredCompositionData
	{
		std::uint32_t m_gbuffer_set;
		std::uint32_t m_skybox_set;
		std::uint32_t m_irradiance_set;
		std::uint32_t m_environment_set;
		std::uint32_t m_brdf_set;
		std::uint32_t m_uav_target_set;
		gfx::DescriptorHeap* m_gbuffer_heap;

		gfx::RootSignature* m_root_sig;
		gfx::PipelineState* m_pipeline;
	};

	namespace internal
	{

		inline void SetupDeferredCompositionTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
			auto context = rs.GetContext();
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::composition);
			auto render_target = fg.GetRenderTarget(handle);

			gfx::RenderTarget* deferred_main_rt;
			if (fg.HasTask<DeferredMainMeshData>())
			{
				deferred_main_rt = fg.GetPredecessorRenderTarget<DeferredMainMeshData>();
			}
			else if (fg.HasTask<DeferredMainData>())
			{
				deferred_main_rt = fg.GetPredecessorRenderTarget<DeferredMainData>();
			}

			gfx::SamplerDesc gbuffer_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_POINT,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			gfx::SamplerDesc skybox_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_ANISOTROPIC,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			gfx::SamplerDesc lut_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_LINEAR,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_CLAMP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			// GPU Heap
			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 8;
			data.m_gbuffer_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);
			data.m_gbuffer_set = data.m_gbuffer_heap->CreateSRVSetFromRT(deferred_main_rt, data.m_root_sig, 1, 0,false, std::nullopt);
			data.m_uav_target_set = data.m_gbuffer_heap->CreateUAVSetFromRT(render_target, 0, data.m_root_sig, 2, 0, gbuffer_sampler_desc);

			// Skybox
			if (fg.HasTask<GenerateCubemapData>())
			{
				auto skybox_rt = fg.GetPredecessorRenderTarget<GenerateCubemapData>();
				data.m_skybox_set = data.m_gbuffer_heap->CreateSRVSetFromRT(skybox_rt, data.m_root_sig, 4, 0, false, skybox_sampler_desc);
			}

			// Irradiance
			if (fg.HasTask<GenerateIrradianceMapData>())
			{
				auto irradiance_rt = fg.GetPredecessorRenderTarget<GenerateIrradianceMapData>();
				data.m_irradiance_set = data.m_gbuffer_heap->CreateSRVSetFromRT(irradiance_rt, data.m_root_sig, 5, 0, false, skybox_sampler_desc);
			}

			// Environment
			if (fg.HasTask<GenerateEnvironmentMapData>())
			{
				auto environmnet_rt = fg.GetPredecessorRenderTarget<GenerateEnvironmentMapData>();
				data.m_environment_set = data.m_gbuffer_heap->CreateSRVSetFromRT(environmnet_rt, data.m_root_sig, 6, 0, false, skybox_sampler_desc);
			}

			// BRDF Lut
			if (fg.HasTask<GenerateBRDFLutData>())
			{
				auto brdf_rt = fg.GetPredecessorRenderTarget<GenerateBRDFLutData>();
				data.m_brdf_set = data.m_gbuffer_heap->CreateSRVSetFromRT(brdf_rt, data.m_root_sig, 7, 0, false, lut_sampler_desc);
			}

			data.m_pipeline = PipelineRegistry::SFind(pipelines::composition);;
		}

		inline void ExecuteDeferredCompositionTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto render_target = fg.GetRenderTarget(handle);
			auto light_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetLightConstantBufferPool());
			auto camera_pool = static_cast<gfx::VkConstantBufferPool*>(sg.GetCameraConstantBufferPool());
			auto light_buffer_handle = sg.GetLightBufferHandle();
			auto camera_handle = sg.m_camera_cb_handles[0].m_value;

			fg.WaitForPredecessorTask<GenerateCubemapData>();


			std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
			{
				{ camera_pool->GetDescriptorHeap(), camera_handle.m_cb_set_id },
				{ data.m_gbuffer_heap, data.m_gbuffer_set },
				{ data.m_gbuffer_heap, data.m_uav_target_set },
				{ light_pool->GetDescriptorHeap(), light_buffer_handle.m_cb_set_id },
				{ data.m_gbuffer_heap, data.m_skybox_set },
				{ data.m_gbuffer_heap, data.m_irradiance_set },
				{ data.m_gbuffer_heap, data.m_environment_set },
				{ data.m_gbuffer_heap, data.m_brdf_set },
			};

			cmd_list->BindComputePipelineState(data.m_pipeline);
			cmd_list->BindComputeDescriptorHeap(data.m_root_sig, sets);
			cmd_list->Dispatch(render_target->GetWidth() / 32, render_target->GetHeight() / 32, 1);
		}

		inline void DestroyDeferredCompositionTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DeferredCompositionData>(handle);
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
			.m_rtv_formats = { VK_FORMAT_R32G32B32A32_SFLOAT },
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

		fg.AddTask<DeferredCompositionData>(desc, L"Deferred Composition Task");
	}

} /* tasks */
