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
#include "../pipeline_registry.hpp"
#include "../root_signature_registry.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../graphics/vk_model_pool.hpp"
#include "../graphics/descriptor_heap.hpp"
#include "../graphics/vk_material_pool.hpp"
#include "../graphics/gfx_enums.hpp"
#include "../graphics/vk_texture_pool.hpp"

namespace tasks
{

	struct GenerateCubemapData
	{
		std::uint32_t m_sky_texture_id;
		std::uint32_t m_input_set;
		std::uint32_t m_uav_target_set;
		gfx::DescriptorHeap* m_desc_heap;

		gfx::RootSignature* m_root_sig;
	};

	namespace internal
	{

		inline void SetupGenerateCubemapTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<GenerateCubemapData>(handle);
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::generate_cubemap);
			auto render_target = fg.GetRenderTarget(handle);

			auto texture_pool = static_cast<gfx::VkTexturePool*>(rs.GetTexturePool());
			data.m_sky_texture_id = texture_pool->Load("gym_4k.hdr", false, false);
			auto textures = texture_pool->GetTextures({ data.m_sky_texture_id });

			gfx::SamplerDesc input_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_LINEAR,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 3;
			data.m_desc_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);
			data.m_input_set = data.m_desc_heap->CreateSRVSetFromTexture(textures, data.m_root_sig, 0, 0, input_sampler_desc);
			data.m_uav_target_set = data.m_desc_heap->CreateUAVSetFromRT(render_target, 0, data.m_root_sig, 1, 0, input_sampler_desc);
		}

		inline void ExecuteGenerateCubemapTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<GenerateCubemapData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto pipeline = PipelineRegistry::SFind(pipelines::generate_cubemap);
			auto render_target = fg.GetRenderTarget(handle);

			std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
			{
				{ data.m_desc_heap, data.m_input_set },
				{ data.m_desc_heap, data.m_uav_target_set },
			};

			cmd_list->BindComputePipelineState(pipeline);
			cmd_list->BindComputeDescriptorHeap(data.m_root_sig, sets);
			cmd_list->Dispatch(render_target->GetWidth() / 8, render_target->GetHeight() / 8, 6);

			fg.SetShouldExecute<GenerateCubemapData>(false);
		}

		inline void DestroyGenerateCubemapTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<GenerateCubemapData>(handle);
			delete data.m_desc_heap;
		}

	} /* internal */

	inline void AddGenerateCubemapTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = 1024,
			.m_height = 1024,
			.m_dsv_format = VK_FORMAT_UNDEFINED,
			.m_rtv_formats = { VK_FORMAT_R32G32B32A32_SFLOAT },
			.m_state_execute = VK_IMAGE_LAYOUT_GENERAL,
			.m_state_finished = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.m_clear = false,
			.m_clear_depth = false,
			.m_allow_direct_access = false,
			.m_is_cube_map = true,
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupGenerateCubemapTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteGenerateCubemapTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyGenerateCubemapTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<GenerateCubemapData>(desc, L"Generate Cubemap Task");
	}

} /* tasks */
