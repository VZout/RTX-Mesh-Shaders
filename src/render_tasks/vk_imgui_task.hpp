/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <imgui.h>

#include "../imgui/imgui_style.hpp"
#include "../imgui/imgui_impl_glfw.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../application.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../renderer.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"

#define IMGUI

namespace tasks
{

	struct ImGuiTaskData
	{
#ifdef IMGUI
		ImGuiImpl* m_imgui_impl;
		bool m_temp;
#endif
	};

	namespace internal
	{

		inline void SetupImGuiTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
#ifdef IMGUI
			if (resize)
			{
				return;
			}

			auto& data = fg.GetData<ImGuiTaskData>(handle);
			auto render_window = fg.GetRenderTarget<gfx::RenderWindow>(handle);
			auto app = rs.GetApp();

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
			io.ConfigDockingWithShift = true;

			ImGui_ImplGlfw_InitForVulkan(app->GetWindow(), true);

			// Setup Dear ImGui style
			ImGui::StyleColorsCherry();

			data.m_imgui_impl = new ImGuiImpl();
			data.m_imgui_impl->InitImGuiResources(rs.GetContext(), render_window, rs.GetDirectQueue());
			LOG("Finished Initializing ImGui");
#endif
		}

		inline void ExecuteImGuiTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
#ifdef IMGUI
			auto& data = fg.GetData<ImGuiTaskData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto frame_idx = rs.GetFrameIdx();

			// imgui itself code
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("About"))
				{
					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			ImGui::Begin("Whatsup");
			ImGui::Text("Hey this is my framerate: %.0f", ImGui::GetIO().Framerate);
			ImGui::ToggleButton("Toggle", &data.m_temp);
			ImGui::End();

			ImGui::Begin("Vulkan Details");

			ImGui::End();

			// Render to generate draw buffers
			ImGui::Render();

			data.m_imgui_impl->UpdateBuffers();
			data.m_imgui_impl->Draw(cmd_list, frame_idx);
#endif
		}

		inline void DestroyImGuiTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<ImGuiTaskData>(handle);

			if (resize)
			{

			}
			else
			{
				delete data.m_imgui_impl;

				ImGui_ImplGlfw_Shutdown();
			}
		}

	} /* internal */

	inline void AddImGuiTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = true,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_clear = false,
			.m_clear_depth = false,
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupImGuiTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			internal::ExecuteImGuiTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyImGuiTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = false;

		fg.AddTask<ImGuiTaskData>(desc, L"ImGui Task");
	}

} /* tasks */
