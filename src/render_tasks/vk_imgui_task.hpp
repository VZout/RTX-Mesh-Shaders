/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <imgui.h>

#include "../imgui/imgui_style.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../application.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../renderer.hpp"
#include "../imgui/imgui_impl_glfw.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../imgui/imgui_gizmo.h"

#define IMGUI

namespace tasks
{

	struct NoTask
	{

	};

	struct ImGuiTaskData
	{
#ifdef IMGUI
		ImGuiImpl* m_imgui_impl;
		bool m_temp;
		util::Delegate<void(ImTextureID)> m_render_func;
		gfx::DescriptorHeap* m_heap;
		ImTextureID m_texture;
#endif
	};

	namespace internal
	{

		template<typename T>
		inline void SetupImGuiTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize, decltype(ImGuiTaskData::m_render_func) render_func)
		{
#ifdef IMGUI
			auto& data = fg.GetData<ImGuiTaskData>(handle);

			if (resize)
			{
				// TODO: Duplicate code.
				gfx::DescriptorHeap::Desc desc;
				desc.m_versions = 1;
				desc.m_num_descriptors = 1;
				data.m_heap = new gfx::DescriptorHeap(rs.GetContext(), desc);
				auto predecessor_rt = fg.GetPredecessorRenderTarget<T>();
				auto texture_desc_set_id = data.m_heap->CreateSRVSetFromRT(predecessor_rt, data.m_imgui_impl->descriptorSetLayout, 0, 0, false);
				data.m_texture = data.m_heap->GetDescriptorSet(0, texture_desc_set_id);

				return;
			}
			auto render_window = fg.GetRenderTarget<gfx::RenderWindow>(handle);
			auto app = rs.GetApp();

			gfx::DescriptorHeap::Desc desc;
			desc.m_versions = 1;
			desc.m_num_descriptors = 1;
			data.m_heap = new gfx::DescriptorHeap(rs.GetContext(), desc);

			data.m_render_func = std::move(render_func);

			// Setup Dear ImGui style
			ImGui::StyleColorsCherry();

			data.m_imgui_impl = new ImGuiImpl();
			data.m_imgui_impl->InitImGuiResources(rs.GetContext(), render_window, rs.GetDirectQueue());

			if constexpr (!std::is_same<T, NoTask>::value)
			{
				auto predecessor_rt = fg.GetPredecessorRenderTarget<T>();
				auto texture_desc_set_id = data.m_heap->CreateSRVSetFromRT(predecessor_rt, data.m_imgui_impl->descriptorSetLayout, 0, 0, false);
				data.m_texture = data.m_heap->GetDescriptorSet(0, texture_desc_set_id);
			}
#endif
		}

		template<typename T>
		inline void ExecuteImGuiTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
#ifdef IMGUI
			auto& data = fg.GetData<ImGuiTaskData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto frame_idx = rs.GetFrameIdx();

			// imgui itself code
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();

			data.m_render_func(data.m_texture);

			// Render to generate draw buffers
			ImGui::Render();

			data.m_imgui_impl->UpdateBuffers(frame_idx);

			if constexpr (!std::is_same<T, NoTask>::value)
			{
				auto predecessor_rt = fg.GetPredecessorRenderTarget<T>();
				cmd_list->TransitionRenderTarget(predecessor_rt, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				cmd_list->BindRenderTargetVersioned(rs.GetRenderWindow());
				data.m_imgui_impl->Draw(cmd_list, frame_idx);
				cmd_list->UnbindRenderTarget();
				cmd_list->TransitionRenderTarget(predecessor_rt, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}
			else
			{
				cmd_list->BindRenderTargetVersioned(rs.GetRenderWindow());
				data.m_imgui_impl->Draw(cmd_list, frame_idx);
				cmd_list->UnbindRenderTarget();
			}
#endif
		}

		inline void DestroyImGuiTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<ImGuiTaskData>(handle);

			delete data.m_heap;

			if (!resize)
			{
				delete data.m_imgui_impl;
			}
		}

	} /* internal */

	template<typename T = ImGuiTaskData>
	inline void AddImGuiTask(fg::FrameGraph& fg, decltype(ImGuiTaskData::m_render_func) const & imgui_render_func)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = true,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_dsv_format = VK_FORMAT_D32_SFLOAT,
			.m_rtv_formats = { gfx::settings::swapchain_format },
			.m_state_execute = std::nullopt,
			.m_state_finished = std::nullopt,
			.m_clear = false,
			.m_clear_depth = false,
			.m_allow_direct_access = false,
			.m_is_cube_map = false,
			.m_mip_levels = 1,
			.m_resolution_scale = 1,
			.m_bind_by_default = false // dont bind by default so we can easily transition
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [imgui_render_func](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupImGuiTask<T>(rs, fg, handle, resize, imgui_render_func);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteImGuiTask<T>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyImGuiTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = false;

		fg.AddTask<ImGuiTaskData>(desc, L"ImGui Task");
	}

} /* tasks */
