

/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include <chrono>

#include <frame_graph/frame_graph.hpp>
#include <application.hpp>
#include <util/version.hpp>
#include <util/user_literals.hpp>
#include <util/browser.hpp>
#include <util/progress.hpp>
#include <render_tasks/vulkan_tasks.hpp>
#include <imgui/icons_font_awesome5.hpp>
#include <imgui/imgui_plot.hpp>
#include <imgui/imgui_gizmo.h>
#include <gtc/type_ptr.hpp>
#include "../common/editor.hpp"
#include "../common/fps_camera.hpp"
#include "../common/frame_graphs.hpp"
#include "../common/spheres_scene.hpp"
#include "../common/sss_scene.hpp"
#include "../common/forrest_scene.hpp"
#include "../common/displacement_scene.hpp"
#include "../common/spaceship_scene.hpp"
#include "../common/market_scene.hpp"

#include <util/cpu_profiler.hpp>

#define DEFAULT_SCENE ForrestScene

class Demo : public Application
{
public:
	Demo()
		: Application("Mesh Shaders Demo"),
		m_renderer(nullptr),
		m_fps_camera()
	{
	}

	~Demo() final
	{
		delete m_empty_scene_graph;
		delete m_loading_frame_graph;
		delete m_frame_graph;
		delete m_scene;
		delete m_renderer;
	}

protected:
	void ImGui_ManipulateNode(sg::Node node, ImGuizmo::OPERATION operation)
	{
		auto model = m_scene->GetSceneGraph()->m_models[node.m_transform_component].m_value;
		auto cam = m_scene->GetSceneGraph()->GetActiveCamera();

		glm::vec3 cam_pos = m_scene->GetSceneGraph()->m_positions[cam.m_transform_component].m_value;
		glm::vec3 cam_rot = m_scene->GetSceneGraph()->m_rotations[cam.m_transform_component].m_value;
		auto aspect_ratio = m_scene->GetSceneGraph()->m_camera_aspect_ratios[cam.m_transform_component].m_value;

		glm::vec3 forward;
		forward.x = cos(cam_rot.y) * cos(cam_rot.x);
		forward.y = sin(cam_rot.x);
		forward.z = sin(cam_rot.y) * cos(cam_rot.x);
		forward = glm::normalize(forward);
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));

		cb::Camera data;
		data.m_view = glm::lookAt(cam_pos, cam_pos + forward, up);
		data.m_proj = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.01f, 1000.0f);

		ImGuizmo::SetRect(m_viewport_pos.x, m_viewport_pos.y, m_viewport_size.x, m_viewport_size.y);
		ImGuizmo::Manipulate(glm::value_ptr(data.m_view), glm::value_ptr(data.m_proj), operation, ImGuizmo::MODE::WORLD, &model[0][0], NULL, NULL);

		float new_translation[3], new_rotation[3], new_scale[3];
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), new_translation, new_rotation, new_scale);

		m_scene->GetSceneGraph()->m_positions[node.m_transform_component].m_value = glm::vec3(new_translation[0], new_translation[1], new_translation[2]);
		m_scene->GetSceneGraph()->m_rotations[node.m_transform_component].m_value = glm::vec3(glm::radians(new_rotation[0]), glm::radians(new_rotation[1]), glm::radians(new_rotation[2]));
		m_scene->GetSceneGraph()->m_scales[node.m_transform_component].m_value = glm::vec3(new_scale[0], new_scale[1], new_scale[2]);
		m_scene->GetSceneGraph()->m_requires_update[node.m_transform_component] = true;
	}

#include "../common/editor_interface.inl"

	void Init() final
	{
		TIME_THIS_SCOPE(Init);

		DisableResizing();

		SetupEditor();
		editor.SetMainMenuBarText("FrameGraph: " + fg_manager::GetFrameGraphName(m_fg_type));

		m_renderer = new Renderer();
		m_renderer->Init(this);

		// Init Loading Screen Resources
		m_empty_scene_graph = new sg::SceneGraph(m_renderer);
		m_loading_frame_graph = fg_manager::CreateFrameGraph(fg_manager::FGType::IMGUI_ONLY, m_renderer, [&](ImTextureID texture)
		{
			ImGuiLoadingScreen();
		});

		m_loading_future = std::async(std::launch::async, [&]()
		{
			SET_NUM_TASKS(m_loading_progress, 4);

			PROGRESS(m_loading_progress, "Allocating Scene");

			m_scene = new DEFAULT_SCENE();

			PROGRESS(m_loading_progress, "Initializing Scene");

			m_scene->Init(m_renderer, m_loading_progress);

			PROGRESS(m_loading_progress, "Setting Up Framegraph");

			m_frame_graph = fg_manager::CreateFrameGraph(m_fg_type, m_renderer, [&](ImTextureID texture)
			{
				editor.SetTexture(texture);
				editor.Render();
			});

			PROGRESS(m_loading_progress, "Setting Up Camera");

			m_fps_camera.SetApplication(this);
			m_fps_camera.SetSceneGraph(m_scene->GetSceneGraph());
			m_fps_camera.SetCameraHandle(m_scene->GetCameraNodeHandle());

			m_should_call_upload = true;
			m_ready_to_render = true;
		});

		m_last = std::chrono::high_resolution_clock::now();
	}

	void Loop() final
	{
		if (m_ready_to_render)
		{
			if (m_loading_future.valid()) m_loading_future.get();
			if (m_should_call_upload)
			{
				m_renderer->Upload();
				m_should_call_upload = false;

				EnableResizing();
			}
			Render();
		}
		else
		{
			RenderLoadingScreen();
		}
	}

	void ImGuiLoadingScreen()
	{
		auto window_size = ImGui::GetMainViewport()->Size;

		ImGui::Begin("Loading Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowSize(window_size);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetCursorPosY(window_size.y / 2.f - 120);

		auto centered_text = [&](std::string text, ImVec4 color) {
			auto text_size = ImGui::CalcTextSize(text.c_str());
			ImGui::SetCursorPosX(window_size.x / 2.f - text_size.x / 2.f);
			ImGui::TextColored(color, text.c_str());
		};

		constexpr auto version = util::GetVersion();

#define TEXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

		ImGui::Separator();
		centered_text(fmt::format("{0}    RTX Mesh Shading    {0}", reinterpret_cast<const char*>(ICON_FA_ROCKET)), ImVec4(1.00f, 0.630f, 0, 1));
		centered_text(fmt::format("{}", util::VersionToString(version)), TEXT(0.6));
		centered_text("Viktor Zoutman", TEXT(0.6));
		ImGui::Separator();

		ImGui::NewLine(); ImGui::NewLine(); ImGui::NewLine(); ImGui::NewLine();

		centered_text(fmt::format("{}  Loading... ({})", reinterpret_cast<const char*>(ICON_FA_CLOCK), m_loading_progress.GetAction()), TEXT(1));

		ImGui::ProgressBar(m_loading_progress.GetFraction());

		std::function<void(util::Progress*)> render_child_progress_bar = [&](util::Progress* progress) -> void
		{
			progress->Lock();
			if (progress->HasChild())
			{
				auto child = progress->GetChild();
				centered_text(fmt::format("{}", child->GetAction()), TEXT(1));

				ImGui::ProgressBar(child->GetFraction());
				render_child_progress_bar(child);
			}
			progress->Unlock();

		};

		render_child_progress_bar(&m_loading_progress);

		ImGui::End();
	}

	void RenderLoadingScreen()
	{
		m_renderer->AquireNewFrame();
		m_renderer->Render(*m_empty_scene_graph, *m_loading_frame_graph);
	}

	void Render()
	{
		TIME_THIS_SCOPE(Frame);

		// Change Frame Graph
		if (m_reload_fg)
		{
			m_renderer->WaitForAllPreviousWork();

			delete m_frame_graph;

			m_frame_graph = fg_manager::CreateFrameGraph(m_fg_type, m_renderer, [this](ImTextureID texture)
			{
				editor.SetTexture(texture);
				editor.Render();
			});

			m_renderer->Upload();

			m_reload_fg = false;
		}

		if (m_reload_sg)
		{
			m_renderer->WaitForAllPreviousWork();

			delete m_scene;
			m_scene = m_new_scene;
			m_new_scene = nullptr;

			m_scene->Init(m_renderer);

			auto scene_graph = m_scene->GetSceneGraph();
			auto camera_node = m_scene->GetCameraNodeHandle();

			m_fps_camera.SetSceneGraph(scene_graph);
			m_fps_camera.SetCameraHandle(camera_node);

			m_renderer->Upload();

			if (!editor.GetEditorVisibility())
			{
				sg::helper::SetAspectRatio(scene_graph, camera_node, (float)GetWidth() / (float)GetHeight());
			}
			else
			{
				sg::helper::SetAspectRatio(scene_graph, camera_node, (float)m_viewport_size.x / (float)m_viewport_size.y);
			}

			m_reload_sg = false;
		}

		if (m_viewport_has_changed)
		{
			//m_renderer->Resize(m_viewport_size.x, m_viewport_size.y, false);
			//m_frame_graph->Resize(m_viewport_size.x, m_viewport_size.y);
			//m_viewport_has_changed = false;
		}

		auto now = std::chrono::high_resolution_clock::now();
		auto diff = now - m_last; 
		m_delta = (float)diff.count() / 1000000000.f; // milliseconds
		m_last = now;

		m_renderer->AquireNewFrame();

		m_scene->Update(m_renderer->GetFrameIdx(), m_delta, m_time);

		m_renderer->Render(*m_scene->GetSceneGraph(), *m_frame_graph);

		m_fps_camera.HandleControllerInput(m_delta);
		m_fps_camera.Update(m_delta);

		m_time += m_delta;
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		if (!m_ready_to_render) return;

		m_renderer->Resize(width, height);
		m_frame_graph->Resize(width, height);

		if (!editor.GetEditorVisibility())
		{
			sg::helper::SetAspectRatio(m_scene->GetSceneGraph(), m_scene->GetCameraNodeHandle(), (float)width / (float)height);
		}
	}

	void SwitchFrameGraph(fg_manager::FGType type)
	{
		if (m_fg_type != type)
		{
			m_selected_task = std::nullopt;

			m_fg_type = type;
			editor.SetMainMenuBarText("FrameGraph: " + fg_manager::GetFrameGraphName(type));
			m_reload_fg = true;
		}
	}

	template<typename T>
	void SwitchScene()
	{
		m_new_scene = new T();
		m_reload_sg = true;
	}

	void MousePosCallback(float x, float y) final
	{
		if (!m_ready_to_render) return;

		m_fps_camera.HandleMousePosition(m_delta, x, y);
	}

	void MouseButtonCallback(int key, int action) final
	{
		if (!m_ready_to_render) return;

		m_fps_camera.HandleMouseButtons(key, action);
	}

	void KeyCallback(int key, int action, int mods) final
	{
		if (!m_ready_to_render) return;

		// Frame graph swithcing
		if (action == GLFW_PRESS && mods & GLFW_MOD_CONTROL)
		{
			if (key == GLFW_KEY_1)
			{
				SwitchFrameGraph(fg_manager::FGType::PBR_GENERIC);
			}
			else if (key == GLFW_KEY_2)
			{
				SwitchFrameGraph(fg_manager::FGType::PBR_MESH_SHADING);
			}
			else if (key == GLFW_KEY_3)
			{
				SwitchFrameGraph(fg_manager::FGType::RAYTRACING);
			}
		}

		// Key Bindings Modal
		if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
		{
			editor.OpenModal("Key Bindings");
		}

		// Editor Visibility
		if ((key == GLFW_KEY_F3 || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
		{
			m_viewport_has_changed = true;

			editor.SetEditorVisibility(!editor.GetEditorVisibility());

			auto scene_graph = m_scene->GetSceneGraph();
			auto camera_node = m_scene->GetCameraNodeHandle();

			if (!editor.GetEditorVisibility())
			{
				sg::helper::SetAspectRatio(scene_graph, camera_node, (float)GetWidth() / (float)GetHeight());
			}
			else
			{
				sg::helper::SetAspectRatio(scene_graph, camera_node, (float)m_viewport_size.x / (float)m_viewport_size.y);
			}
		}

		if ((!ImGui::GetIO().WantCaptureKeyboard || m_viewport_has_focus) && !m_fps_camera.IsControlled())
		{
			if (key == GLFW_KEY_W && action == GLFW_PRESS)
			{
				m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
			}
			else if (key == GLFW_KEY_E && action == GLFW_PRESS)
			{
				m_gizmo_operation = ImGuizmo::OPERATION::ROTATE;
			}
			else if (key == GLFW_KEY_R && action == GLFW_PRESS)
			{
				m_gizmo_operation = ImGuizmo::OPERATION::SCALE;
			}
		}

		m_fps_camera.HandleKeyboardInput(key, action);
	}

	Editor editor;
	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;

	bool m_ready_to_render = false;
	bool m_should_call_upload = false;
	sg::SceneGraph* m_empty_scene_graph;
	fg::FrameGraph* m_loading_frame_graph;
	std::future<void> m_loading_future;
	util::Progress m_loading_progress;

	bool m_reload_fg = false;
	bool m_reload_sg = false;

	Scene* m_scene;
	Scene* m_new_scene;

	float m_delta;
	float m_time = 0;

	// Camera Movement
	FPSCamera m_fps_camera;

	ImGuizmo::OPERATION m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

	// ImGui
	std::chrono::time_point<std::chrono::high_resolution_clock> m_last;
	std::optional<sg::NodeHandle> m_selected_node;
	std::optional<std::uint32_t> m_selected_task;
	ImGuiTextFilter m_outliner_filter;
	bool m_viewport_has_focus = false;
	ImVec2 m_viewport_pos = { 0, 0 };
	ImVec2 m_viewport_size = { 1280, 720 };

	bool m_viewport_has_changed = false;

	fg_manager::FGType m_fg_type = fg_manager::FGType::RAYTRACING;
};


ENTRY(Demo, 1280, 720)
