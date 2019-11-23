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
#include "../graphics/vk_texture_pool.hpp"
#include "vk_generate_cubemap.hpp"

namespace tasks
{

	struct GenerateEnvironmentMapData
	{
		std::uint32_t m_input_set;
		std::vector<std::uint32_t> m_uav_target_sets;
		gfx::DescriptorHeap* m_desc_heap;

		gfx::RootSignature* m_root_sig;
	};

	namespace internal
	{

		inline void SetupGenerateEnvironmentMapTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<GenerateEnvironmentMapData>(handle);
			data.m_root_sig = RootSignatureRegistry::SFind(root_signatures::generate_environmentmap);
			auto render_target = fg.GetRenderTarget(handle);

			gfx::SamplerDesc input_sampler_desc
			{
				.m_filter = gfx::enums::TextureFilter::FILTER_ANISOTROPIC,
				.m_address_mode = gfx::enums::TextureAddressMode::TAM_CLAMP,
				.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
			};

			auto num_mips = render_target->GetMipLevels();

			gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
			descriptor_heap_desc.m_versions = 1;
			descriptor_heap_desc.m_num_descriptors = 2 + num_mips;
			data.m_desc_heap = new gfx::DescriptorHeap(rs.GetContext(), descriptor_heap_desc);

			data.m_uav_target_sets.resize(num_mips);
			for (auto i = 0; i < num_mips; i++)
			{
				data.m_uav_target_sets[i] = data.m_desc_heap->CreateUAVSetFromRT(render_target, 0, data.m_root_sig, 1, 0, input_sampler_desc, i);
			}

			// Skybox
			if (fg.HasTask<GenerateCubemapData>())
			{
				auto skybox_rt = fg.GetPredecessorRenderTarget<GenerateCubemapData>();
				data.m_input_set = data.m_desc_heap->CreateSRVSetFromRT(skybox_rt, data.m_root_sig, 0, 0, false, input_sampler_desc);
			}
		}

		inline void ExecuteGenerateEnvironmentMapTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<GenerateEnvironmentMapData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto pipeline = PipelineRegistry::SFind(pipelines::generate_environmentmap);
			auto render_target = fg.GetRenderTarget(handle);
			auto num_mips = render_target->GetMipLevels();

			cmd_list->BindPipelineState(pipeline);

			for (std::uint32_t i = 0; i < num_mips; i++)
			{
				// For every mip depth perform a filter operation.
				for (std::uint32_t f = 0; f < 6; f++)
				{
					// Bind the correct descriptor sets. (target is depended on the mip)
					std::vector<std::pair<gfx::DescriptorHeap*, std::uint32_t>> sets
					{
						{data.m_desc_heap, data.m_input_set},
						{data.m_desc_heap, data.m_uav_target_sets[i]}
					};
					cmd_list->BindDescriptorHeap(data.m_root_sig, sets);

					// Update the cb with the correct info.
					struct PushBlock
					{
						float roughness;
						float face;
					} push_data;
					push_data.roughness = (float) i / (float) (num_mips - 1);
					push_data.face = f;

					cmd_list->BindComputePushConstants(data.m_root_sig, &push_data, sizeof(PushBlock));

					cmd_list->Dispatch(render_target->GetWidth() / 32, render_target->GetHeight() / 32, 1);
				}
			}

			fg.SetShouldExecute<GenerateEnvironmentMapData>(false);
		}

		inline void DestroyGenerateEnvironmentMapTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<GenerateEnvironmentMapData>(handle);
			delete data.m_desc_heap;
		}

	} /* internal */

	inline void AddGenerateEnvironmentMapTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = false,
			.m_width = 512,
			.m_height = 512,
			.m_dsv_format = VK_FORMAT_UNDEFINED,
			.m_rtv_formats = { VK_FORMAT_R16G16B16A16_SFLOAT },
			.m_state_execute = VK_IMAGE_LAYOUT_GENERAL,
			.m_state_finished = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.m_clear = false,
			.m_clear_depth = false,
			.m_allow_direct_access = false,
			.m_is_cube_map = true,
			.m_mip_levels = static_cast<std::uint32_t>(std::floor(std::log2(rt_properties.m_width.value()))) + 1
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupGenerateEnvironmentMapTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteGenerateEnvironmentMapTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyGenerateEnvironmentMapTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<GenerateEnvironmentMapData>(desc, L"Generate Environment map Task");
	}

} /* tasks */
