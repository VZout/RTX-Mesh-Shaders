/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include <chrono>

#include "frame_graph/frame_graph.hpp"
#include "application.hpp"
#include "../common/editor.hpp"
#include "../common/fps_camera.hpp"
#include "../common/frame_graphs.hpp"
#include "util/version.hpp"
#include "util/user_literals.hpp"
#include "render_tasks/vulkan_tasks.hpp"
#include "imgui/icons_font_awesome5.hpp"
#include "imgui/imgui_plot.hpp"
#include "imgui/imgui_gizmo.h"
#include <gtc/type_ptr.hpp>

#define MESH_SHADING

#ifdef _WIN32
#include <shellapi.h>
#endif

inline void OpenURL(std::string url)
{
#ifdef _WIN32
	ShellExecuteA(0, 0, url.c_str(), 0, 0 , SW_SHOW );
#endif
}

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
		delete m_frame_graph;
		delete m_scene_graph;
		delete m_renderer;
	}

protected:
	void ImGui_ManipulateNode(sg::Node node, ImGuizmo::OPERATION operation)
	{
		auto model = m_scene_graph->m_models[node.m_transform_component].m_value;
		auto cam = m_scene_graph->GetActiveCamera();

		glm::vec3 cam_pos = m_scene_graph->m_positions[cam.m_transform_component].m_value;
		glm::vec3 cam_rot = m_scene_graph->m_rotations[cam.m_transform_component].m_value;
		auto aspect_ratio = m_scene_graph->m_camera_aspect_ratios[cam.m_transform_component].m_value;

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

		m_scene_graph->m_positions[node.m_transform_component].m_value = glm::vec3(new_translation[0], new_translation[1], new_translation[2]);
		m_scene_graph->m_rotations[node.m_transform_component].m_value = glm::vec3(glm::radians(new_rotation[0]), glm::radians(new_rotation[1]), glm::radians(new_rotation[2]));
		m_scene_graph->m_scales[node.m_transform_component].m_value = glm::vec3(new_scale[0], new_scale[1], new_scale[2]);
		m_scene_graph->m_requires_update[node.m_transform_component] = true;
	}

#include "../common/editor_interface.inl"

	void Init() final
	{
		SetupEditor();
		editor.SetMainMenuBarText("FrameGraph: " + fg_manager::GetFrameGraphName(m_fg_type));

		m_renderer = new Renderer();
		m_renderer->Init(this);

		ExtraMaterialData extra_material_data;
		extra_material_data.m_thickness_texture_paths = {
			"bigdude_custom/RGB.png",
		};

		auto model_pool = m_renderer->GetModelPool();
		auto texture_pool = m_renderer->GetTexturePool();
		m_material_pool = m_renderer->GetMaterialPool();
		m_robot_model_handle = model_pool->LoadWithMaterials<Vertex>("bigdude_custom/PBR - Metallic Roughness SSS.gltf", m_material_pool, texture_pool, false, extra_material_data);
		auto sphere_model_handle = model_pool->LoadWithMaterials<Vertex>("sphere.fbx", m_material_pool, texture_pool, false);

		float num_spheres_x = 9;
		float num_spheres_y = 9;
		m_sphere_materials.resize(num_spheres_x * num_spheres_y);
		m_sphere_material_handles.resize(num_spheres_x * num_spheres_y);

		int i = 0;
		for (auto x = 0; x < num_spheres_x; x++)
		{
			for (auto y = 0; y < num_spheres_y; y++)
			{
				MaterialData mat{};

				mat.m_base_color[0] = 1.000;
				mat.m_base_color[1] = 0.766;
				mat.m_base_color[2] = 0.336;
				mat.m_base_roughness = x / num_spheres_x;
				mat.m_base_metallic = y / num_spheres_y;
				mat.m_base_reflectivity = 0.5f;

				m_sphere_material_handles[i] = m_material_pool->Load(mat, texture_pool);
				m_sphere_materials[i] = mat;

				i++;
			}
		}

		m_frame_graph = fg_manager::CreateFrameGraph(m_fg_type, m_renderer, [this](ImTextureID texture)
		{
			editor.SetTexture(texture);
			editor.Render();
		});

		m_renderer->Upload();

		m_scene_graph = new sg::SceneGraph(m_renderer);

		m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
		sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0, 0, 8.2f));
		sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(0, -90._deg, 0));

		m_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_robot_model_handle);
		sg::helper::SetPosition(m_scene_graph, m_node, glm::vec3(-0.75, -1, 0));
		sg::helper::SetScale(m_scene_graph, m_node, glm::vec3(1, 1, 1));
		sg::helper::SetRotation(m_scene_graph, m_node, glm::vec3(0._deg, 0, 0));

		// second node
		{
			float spacing = 2.1f * 0.4f;
			int i = 0;
			for (auto x = 0; x < num_spheres_x; x++)
			{
				for (auto y = 0; y < num_spheres_y; y++)
				{
					float start_x = -spacing * (num_spheres_x / 2);
					float start_y = -spacing * (num_spheres_y / 2);

					auto sphere_node = m_scene_graph->CreateNode<sg::MeshComponent>(sphere_model_handle);
					sg::helper::SetPosition(m_scene_graph, sphere_node, { start_x + (spacing * x), start_y + (spacing * y), 0 });
					sg::helper::SetScale(m_scene_graph, sphere_node, glm::vec3(0.4, 0.4, 0.4));
					sg::helper::SetMaterial(m_scene_graph, sphere_node, { m_sphere_material_handles[i] });

					i++;
				}
			}
		}

		// light node
		m_light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(20, 20, 20));
		sg::helper::SetPosition(m_scene_graph, m_light_node, glm::vec3(0, 0, 2));
		sg::helper::SetRadius(m_scene_graph, m_light_node, 4);

		m_last = std::chrono::high_resolution_clock::now();

		m_fps_camera.SetApplication(this);
		m_fps_camera.SetSceneGraph(m_scene_graph);
		m_fps_camera.SetCameraHandle(m_camera_node);
	}

	void Loop() final
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto diff = now - m_last; 
		m_delta = (float)diff.count() / 1000000000.f; // milliseconds
		m_last = now;

		// animate light
		float light_x = sin(m_time * 2) * 2;
		float light_y = cos(m_time * 2) * 2;
		sg::helper::SetPosition(m_scene_graph, m_light_node, glm::vec3(light_x, light_y, 2));

		m_scene_graph->Update(m_renderer->GetFrameIdx());
		m_renderer->Render(*m_scene_graph, *m_frame_graph);

		auto append_graph_list = [](auto& list, auto time, auto max, auto& lowest, auto& highest)
		{
			lowest = time < lowest ? time : lowest;
			highest = time > highest ? time : highest;

			if (static_cast<int>(list.size()) > max) //Max seconds to show
			{
				for (size_t i = 1; i < list.size(); i++)
				{
					list[i - 1] = list[i];
				}
				list[list.size() - 1] = time;
			} else
			{
				list.push_back(time);
			}
		};

		append_graph_list(m_frame_rates, ImGui::GetIO().Framerate, m_max_frame_rates, m_min_frame_rate,
		                  m_max_frame_rate);

		HandleControllerInput();

		m_fps_camera.HandleControllerInput(m_delta);
		m_fps_camera.Update(m_delta);

		m_time += m_delta;
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		m_renderer->Resize(width, height);
		m_frame_graph->Resize(width, height);

		if (!editor.GetEditorVisibility())
		{
			sg::helper::SetAspectRatio(m_scene_graph, m_camera_node, (float)width / (float)height);
		}
	}

	void HandleControllerInput()
	{

	}

	void MousePosCallback(float x, float y) final
	{
		m_fps_camera.HandleMousePosition(m_delta, x, y);
	}

	void MouseButtonCallback(int key, int action) final
	{
		m_fps_camera.HandleMouseButtons(key, action);
	}

	void KeyCallback(int key, int action) final
	{
		// Editor Visibility
		if ((key == GLFW_KEY_F3 || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
		{
			editor.SetEditorVisibility(!editor.GetEditorVisibility());

			if (!editor.GetEditorVisibility())
			{
				sg::helper::SetAspectRatio(m_scene_graph, m_camera_node, (float)GetWidth() / (float)GetHeight());
			}
			else
			{
				sg::helper::SetAspectRatio(m_scene_graph, m_camera_node, (float)m_viewport_size.x / (float)m_viewport_size.y);
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
	sg::SceneGraph* m_scene_graph;

	sg::NodeHandle m_node;
	sg::NodeHandle m_camera_node;
	sg::NodeHandle m_light_node;

	MaterialPool* m_material_pool;
	ModelHandle m_robot_model_handle;
	MaterialData m_temp_debug_mat_data;
	bool m_imgui_override_color = false;
	bool m_imgui_override_roughness = false;
	bool m_imgui_override_metallic = false;
	bool m_imgui_override_reflectivity = false;
	bool m_imgui_disable_normal_mapping = false;
	float m_ball_reflectivity = 0.5;
	float m_ball_anisotropy = 0;
	glm::vec3 m_ball_anisotropy_dir = { 1, 0, 0 };
	float m_ball_clear_coat = 0;
	float m_ball_clear_coat_roughness = 0;

	std::vector<sg::NodeHandle> m_sphere_nodes;
	std::vector<MaterialHandle> m_sphere_material_handles;
	std::vector<MaterialData> m_sphere_materials;

	float m_delta;
	float m_time = 0;

	// Camera Movement
	FPSCamera m_fps_camera;

	ImGuizmo::OPERATION m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

	// ImGui
	std::chrono::time_point<std::chrono::high_resolution_clock> m_last;
	std::vector<float> m_frame_rates;
	int m_max_frame_rates = 1000;
	float m_min_frame_rate = 0;
	float m_max_frame_rate = 1;
	std::optional<sg::NodeHandle> m_selected_node;
	ImGuiTextFilter m_outliner_filter;
	bool m_viewport_has_focus = false;
	ImVec2 m_viewport_pos = { 0, 0 };
	ImVec2 m_viewport_size = { 1280, 720};

#ifdef MESH_SHADING
	fg_manager::FGType m_fg_type = fg_manager::FGType::PBR_MESH_SHADING;
#else 
	fg_manager::FGType m_fg_type = fg_manager::FGType::PBR_GENERIC;
#endif // MESH SHADING
};


ENTRY(Demo, 1280, 720)
