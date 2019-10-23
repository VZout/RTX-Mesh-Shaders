/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include <chrono>

#include "frame_graph/frame_graph.hpp"
#include "application.hpp"
#include "../common/editor.hpp"
#include "util/version.hpp"
#include "util/user_literals.hpp"
#include "render_tasks/vulkan_tasks.hpp"
#include "imgui/icons_font_awesome5.hpp"
#include "imgui/imgui_plot.hpp"
#include "imgui/imgui_gizmo.h"
#include <gtc/type_ptr.hpp>

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
		m_renderer(nullptr)
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

		glm::vec3 forward;
		forward.x = cos(cam_rot.y) * cos(cam_rot.x);
		forward.y = sin(cam_rot.x);
		forward.z = sin(cam_rot.y) * cos(cam_rot.x);
		forward = glm::normalize(forward);
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));

		cb::Camera data;
		data.m_view = glm::lookAt(cam_pos, cam_pos + forward, up);
		data.m_proj = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.01f, 1000.0f);

		ImGuizmo::SetRect(0, 0, GetWidth(), GetHeight());
		ImGuizmo::Manipulate(glm::value_ptr(data.m_view), glm::value_ptr(data.m_proj), operation, ImGuizmo::MODE::LOCAL, &model[0][0], NULL, NULL);

		float new_translation[3], new_rotation[3], new_scale[3];
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), new_translation, new_rotation, new_scale);

		m_scene_graph->m_positions[node.m_transform_component].m_value = glm::vec3(new_translation[0], new_translation[1], new_translation[2]);
		m_scene_graph->m_rotations[node.m_transform_component].m_value = glm::vec3(glm::radians(new_rotation[0]), glm::radians(new_rotation[1]), glm::radians(new_rotation[2]));
		m_scene_graph->m_scales[node.m_transform_component].m_value = glm::vec3(new_scale[0], new_scale[1], new_scale[2]);
		m_scene_graph->m_requires_update[node.m_transform_component] = true;
	}

	void SetupEditor()
	{
		// Categories
		editor.RegisterCategory("File", reinterpret_cast<const char*>(ICON_FA_FILE));
		editor.RegisterCategory("Scene Graph", reinterpret_cast<const char*>(ICON_FA_PROJECT_DIAGRAM));
		editor.RegisterCategory("Stats", reinterpret_cast<const char*>(ICON_FA_CHART_BAR));
		editor.RegisterCategory("Debug", reinterpret_cast<const char*>(ICON_FA_BUG));
		editor.RegisterCategory("Help", reinterpret_cast<const char*>(ICON_FA_INFO_CIRCLE));

		// Actions
		editor.RegisterAction("Quit", "File", [&](){ Close(); }, reinterpret_cast<const char*>(ICON_FA_POWER_OFF));
		editor.RegisterAction("Contribute", "Help", [&](){ OpenURL("https://github.com/VZout/RTX-Mesh-Shaders"); });
		editor.RegisterAction("Report Issue", "Help", [&](){ OpenURL("https://github.com/VZout/RTX-Mesh-Shaders/issues"); }, reinterpret_cast<const char*>(ICON_FA_BUG));

		// Windows
		editor.RegisterWindow("World Outliner", "Scene Graph", [&]()
		{
			auto gizmo_button = [&](auto icon, auto operation, auto tooltip)
			{
				bool selected = m_gizmo_operation == operation;

				if (selected)
				{
					auto active_color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
					ImGui::PushStyleColor(ImGuiCol_Button, active_color);
				}

				if (ImGui::Button(reinterpret_cast<const char*>(icon)))
				{
					m_gizmo_operation = operation;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text(tooltip);
					ImGui::EndTooltip();
				}

				if (selected)
				{
					ImGui::PopStyleColor();
				}
			};

			gizmo_button(ICON_FA_ARROWS_ALT, ImGuizmo::OPERATION::TRANSLATE, "Translate (W)");
			ImGui::SameLine();
			gizmo_button(ICON_FA_SYNC_ALT, ImGuizmo::OPERATION::ROTATE, "Rotate (E)");
			ImGui::SameLine();
			gizmo_button(ICON_FA_EXPAND_ARROWS_ALT, ImGuizmo::OPERATION::SCALE, "Scale (R)");

			ImGui::SameLine();

			m_outliner_filter.Draw("##");

			ImVec2 size = ImGui::GetContentRegionAvail();
			if (ImGui::ListBoxHeader("##", size))
			{
				const auto& node_handles = m_scene_graph->GetNodeHandles();
				for (std::size_t i = 0; i < node_handles.size(); i++)
				{
					auto handle = node_handles[i];
					auto node = m_scene_graph->GetNode(handle);

					std::string name_prefix = "Unknown Node";
					if (node.m_mesh_component > -1)
					{
						name_prefix = fmt::format("{} Mesh Node", reinterpret_cast<const char*>(ICON_FA_CUBE));
					}
					else if (node.m_camera_component > -1)
					{
						name_prefix = fmt::format("{} Camera Node", reinterpret_cast<const char*>(ICON_FA_VIDEO));
					}
					else if (node.m_light_component > -1)
					{
						name_prefix = fmt::format("{} ", reinterpret_cast<const char*>(ICON_FA_LIGHTBULB));
						switch (m_scene_graph->m_light_types[node.m_light_component])
						{
						case cb::LightType::POINT: name_prefix += "Point Light Node"; break;
						case cb::LightType::DIRECTIONAL: name_prefix += "Directional Light Node"; break;
						case cb::LightType::SPOT: name_prefix += "Spotlight Node"; break;
						default: name_prefix += "Unknown Light Node"; break;
						}
					}

					auto node_name = name_prefix + " (" + std::to_string(i) + ")";

					if (!m_outliner_filter.PassFilter(node_name.c_str())) continue;

					bool pressed = ImGui::Selectable(node_name.c_str(), m_selected_node == handle);
					if (pressed)
					{
						m_selected_node = handle;
					}
				}
				ImGui::ListBoxFooter();
			}

			if (m_selected_node.has_value())
			{
				auto node = m_scene_graph->GetNode(m_selected_node.value());
				if (node.m_camera_component == -1)
				{
					ImGui_ManipulateNode(node, m_gizmo_operation);
				}
			}
		}, false, reinterpret_cast<const char*>(ICON_FA_GLOBE_EUROPE));

		editor.RegisterWindow("Temporary Material Settings", "Scene Graph", [&]()
		{
			ImGui::DragFloat("Ball Reflectivity", &m_ball_reflectivity, 0.01, -0, 1);
			ImGui::DragFloat("Ball Anisotropy", &m_ball_anisotropy, 0.01, -1, 1);
			ImGui::DragFloat3("Ball Anisotropy Dir", reinterpret_cast<float*>(&m_ball_anisotropy_dir), 0.01, -1, 1);
			ImGui::DragFloat("Ball Clear Coat", &m_ball_clear_coat, 0.01, 0, 1);
			ImGui::DragFloat("Ball Clear Coat Roughness", &m_ball_clear_coat_roughness, 0.01, 0, 1);

			ImGui::Separator();

			ImGui::ToggleButton("Override Color", &m_imgui_override_color);
			ImGui::ColorPicker3("Color", m_temp_debug_mat_data.m_base_color, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);

			ImGui::Separator();

			ImGui::Checkbox("Disable Normal Mapping", &m_imgui_disable_normal_mapping);
			ImGui::DragFloat("Normal Strength", &m_temp_debug_mat_data.m_base_normal_strength, 0.01, 0, 10);

			ImGui::Separator();

			ImGui::Checkbox("##0", &m_imgui_override_roughness); ImGui::SameLine();
			ImGui::DragFloat("Roughness", &m_temp_debug_mat_data.m_base_roughness, 0.01, 0, 1);
			ImGui::Checkbox("##1", &m_imgui_override_metallic); ImGui::SameLine();
			ImGui::DragFloat("Metallic", &m_temp_debug_mat_data.m_base_metallic, 0.01, -0, 1);
			ImGui::Checkbox("##2", &m_imgui_override_reflectivity); ImGui::SameLine();
			ImGui::DragFloat("Reflectivity", &m_temp_debug_mat_data.m_base_reflectivity, 0.01, -0, 1);
			ImGui::DragFloat("Anisotropy ", &m_temp_debug_mat_data.m_base_anisotropy, 0.01, -1, 1);
			ImGui::DragFloat("Clear Coat", &m_temp_debug_mat_data.m_base_clear_coat, 0.01, 0, 1);
			ImGui::DragFloat("Clear Coat Roughness", &m_temp_debug_mat_data.m_base_clear_coat_roughness, 0.01, 0, 1);

			m_temp_debug_mat_data.m_base_normal_strength = m_imgui_disable_normal_mapping ? -1 : m_temp_debug_mat_data.m_base_normal_strength;
			m_temp_debug_mat_data.m_base_color[0] = m_imgui_override_color ? m_temp_debug_mat_data.m_base_color[0] : -1;
			m_temp_debug_mat_data.m_base_roughness = m_imgui_override_roughness ? m_temp_debug_mat_data.m_base_roughness : -1;
			m_temp_debug_mat_data.m_base_metallic = m_imgui_override_metallic ? m_temp_debug_mat_data.m_base_metallic : -1;
			m_temp_debug_mat_data.m_base_reflectivity = m_imgui_override_reflectivity ? m_temp_debug_mat_data.m_base_reflectivity : -1;

			int i = 0;
			for (auto mesh_handle : m_sphere_material_handles)
			{
				m_sphere_materials[i].m_base_color[0] = m_imgui_override_color ? m_temp_debug_mat_data.m_base_color[0] : -1;
				m_sphere_materials[i].m_base_color[1] = m_imgui_override_color ? m_temp_debug_mat_data.m_base_color[1] : -1;
				m_sphere_materials[i].m_base_color[2] = m_imgui_override_color ? m_temp_debug_mat_data.m_base_color[2] : -1;

				m_sphere_materials[i].m_base_reflectivity = m_ball_reflectivity;
				m_sphere_materials[i].m_base_anisotropy = m_ball_anisotropy;
				m_sphere_materials[i].m_base_clear_coat = m_ball_clear_coat;
				m_sphere_materials[i].m_base_clear_coat_roughness = m_ball_clear_coat_roughness;
				m_material_pool->Update(mesh_handle, m_sphere_materials[i]);
				m_sphere_materials[i].m_base_anisotropy_dir = m_ball_anisotropy_dir;
				i++;
			}

			for (auto mesh_handle : m_robot_model_handle.m_mesh_handles)
			{
				m_material_pool->Update(mesh_handle.m_material_handle.value(), m_temp_debug_mat_data);
			}

		});

		editor.RegisterWindow("Inspector", "Scene Graph", [&]()
		{
			if (!m_selected_node.has_value()) return;

			auto node = m_scene_graph->GetNode(m_selected_node.value());

			if (node.m_transform_component > -1)
			{
				if (node.m_light_component == -1 || m_scene_graph->m_light_types[node.m_light_component] != cb::LightType::DIRECTIONAL)
				{
					ImGui::DragFloat3("Position", &m_scene_graph->m_positions[node.m_transform_component].m_value[0], 0.1f);
				}

				if (node.m_light_component == -1 || m_scene_graph->m_light_types[node.m_light_component] != cb::LightType::POINT)
				{
					auto euler = glm::degrees(m_scene_graph->m_rotations[node.m_transform_component].m_value);
					ImGui::DragFloat3("Rotation", &euler[0], 0.1f);
					m_scene_graph->m_rotations[node.m_transform_component].m_value = glm::radians(euler);
				}

				if (node.m_camera_component == -1 && node.m_light_component == -1)
				{
					ImGui::DragFloat3("Scale", &m_scene_graph->m_scales[node.m_transform_component].m_value[0], 0.01f);
				}
			}

			if (node.m_light_component > -1)
			{
				const char* types[] = {
					"Point Light",
					"Directional Light",
					"Spotlight"
				};

				ImGui::Separator();
				int selected_type = (int)m_scene_graph->m_light_types[node.m_light_component].m_value;
				ImGui::Combo("Type", &selected_type, types, 3);
				m_scene_graph->m_light_types[node.m_light_component].m_value = (cb::LightType)selected_type;

				ImGui::DragFloat3("Color", &m_scene_graph->m_colors[node.m_light_component].m_value[0], 0.1f);

				if (m_scene_graph->m_light_types[node.m_light_component] == cb::LightType::POINT)
				{
					ImGui::DragFloat("Radius", &m_scene_graph->m_radius[node.m_light_component].m_value, 0.05f);
				}

				if (m_scene_graph->m_light_types[node.m_light_component] == cb::LightType::SPOT)
				{
					auto inner = glm::degrees(m_scene_graph->m_light_angles[node.m_light_component].m_value.first);
					auto outer = glm::degrees(m_scene_graph->m_light_angles[node.m_light_component].m_value.second);

					ImGui::DragFloat("Inner Angle", &inner);
					ImGui::DragFloat("Outer Angle", &outer);

					m_scene_graph->m_light_angles[node.m_light_component].m_value.first = glm::radians(inner);
					m_scene_graph->m_light_angles[node.m_light_component].m_value.second = glm::radians(outer);
				}
			}

			m_scene_graph->m_requires_update[node.m_transform_component] = true;
		}, false, reinterpret_cast<const char*>(ICON_FA_EYE));

		editor.RegisterWindow("Performance", "Stats", [&]()
		{
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 100);
			ImGui::Text("Delta: %.6f", m_delta);
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::NextColumn();
			ImGui::InputInt("Max Samples", &m_max_frame_rates);
			ImGui::SameLine();
			if (ImGui::Button("Reset Scale"))
			{
			  m_max_frame_rate = 1;
			  m_min_frame_rate = 0;
			}
			ImGui::Columns(1);

			ImGui::PlotConfig conf;
			conf.values.ys = m_frame_rates.data();
			conf.values.count = m_frame_rates.size();
			conf.scale.min = m_min_frame_rate;
			conf.values.color = ImColor(0, 255, 0);
			conf.scale.max = m_max_frame_rate;
			conf.tooltip.show = true;
			conf.tooltip.format = "fps=%.2f";
			conf.grid_x.show = false;
			conf.grid_y.show = false;
			conf.frame_size = ImGui::GetContentRegionAvail();
			conf.line_thickness = 3.f;

			ImGui::Plot("plot", conf);
		});

		editor.RegisterWindow("GPU Info", "Stats", [&]()
		{
			auto context = m_renderer->GetContext();
			auto device_properties = context->GetPhysicalDeviceProperties();
			auto device_mem_properties = context->GetPhysicalDeviceMemoryProperties();

			VkDeviceSize vram = 0;
			for (auto i = 0; i < device_mem_properties->memoryHeapCount; i++)
			{
				if (device_mem_properties->memoryHeaps[i].flags == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				{
					vram = device_mem_properties->memoryHeaps[i].size;
				}
			}

			std::string type_str = "Unknown";
			auto type = device_properties.deviceType;
			switch (type)
			{
			default: break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: type_str = "CPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: type_str = "Discrete GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_OTHER: type_str = "Other"; break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: type_str = "Integrated GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: type_str = "Virtual GPU"; break;
			}
			
			ImGui::InfoText("Name", std::string(device_properties.deviceName));
			ImGui::InfoText("Device Type", type_str);
			ImGui::InfoText("VRAM", std::to_string(vram / 1024.f / 1024.f / 1024.f) + " (GB)");
			ImGui::InfoText("API Version", device_properties.apiVersion);
			ImGui::InfoText("Driver Version", device_properties.driverVersion);
			ImGui::InfoText("Device ID", device_properties.deviceID);
			ImGui::InfoText("Vendor ID", device_properties.vendorID);
		});

		editor.RegisterWindow("Memory Allocator Stats", "Stats", [&]()
		{
			auto context = m_renderer->GetContext();
			auto stats = context->CalculateVMAStats();

			auto render_vma_stat_info = [](VmaStatInfo& stat_info)
			{
				ImGui::InfoText("Used Bytes", stat_info.usedBytes);
				ImGui::InfoText("Unused Bytes", stat_info.unusedBytes);
				ImGui::InfoText("Allocation Count", stat_info.allocationCount);
				ImGui::InfoText("Allocation Size Avg", stat_info.allocationSizeAvg);
				ImGui::InfoText("Allocation Size Max", stat_info.allocationSizeMax);
				ImGui::InfoText("Allocation Size Min", stat_info.allocationSizeMin);
				ImGui::InfoText("Block Count", stat_info.blockCount);
				ImGui::InfoText("Unused Range Count", stat_info.unusedRangeCount);
				ImGui::InfoText("Unused Range Size Avg", stat_info.unusedRangeSizeAvg);
				ImGui::InfoText("Unused Range Size Max", stat_info.unusedRangeSizeMax);
				ImGui::InfoText("Unused Range Size Min", stat_info.unusedRangeSizeMin);
			};

			if (ImGui::CollapsingHeader("Total"))
			{
				render_vma_stat_info(stats.total);
			}

			if (ImGui::CollapsingHeader("Memory Heaps"))
			{
				for (auto i = 0; i < VK_MAX_MEMORY_HEAPS; i++)
				{
					auto device_mem_properties = context->GetPhysicalDeviceMemoryProperties();

					auto stat_info = stats.memoryHeap[i];
					if (stat_info.allocationCount < 1) continue; // dont show mem info without allocations 

					std::string name = "Heap " + std::to_string(i);

					if (ImGui::TreeNode(name.c_str()))
					{
						render_vma_stat_info(stat_info);
						ImGui::TreePop();
					}
				}
			}

			if (ImGui::CollapsingHeader("Memory Types"))
			{
				for (auto i = 0; i < VK_MAX_MEMORY_TYPES; i++)
				{
					auto stat_info = stats.memoryType[i];
					if (stat_info.allocationCount < 1) continue; // dont show mem info without allocations 

					std::string name = "Type " + std::to_string(i);

					if (ImGui::TreeNode(name.c_str()))
					{
						render_vma_stat_info(stat_info);
						ImGui::TreePop();
					}
				}
			}
		});

		editor.RegisterWindow("Input Settings", "Debug", [&]()
		{
			ImGui::DragFloat("Movement Speed", &m_move_speed, 1, 0, 100);
			ImGui::DragFloat("Mouse Sensitivity", &m_mouse_sensitivity, 1, 0, 100);
			ImGui::DragFloat("Controller Sensitivity", &m_controller_sensitivity, 1, 0, 100);
			ImGui::ToggleButton("Inverted Controller Y", &m_flip_controller_y);
		});

		editor.RegisterWindow("About", "Help", [&]()
		{
			ImGui::Text("Turing Mesh Shading");
			constexpr auto version = util::GetVersion();
			std::string version_text = "Version: " + util::VersionToString(version);
			ImGui::Text("%s", version_text.c_str());
			ImGui::Separator();
			ImGui::Text("Copyright 2019 Viktor Zoutman");
			if (ImGui::Button("License")) OpenURL("https://github.com/VZout/RTX-Mesh-Shaders/blob/master/LICENSE");
			ImGui::SameLine();
			if (ImGui::Button("Portfolio")) OpenURL("http://www.vzout.com/");
		});
	}

	void Init() final
	{
		SetupEditor();


		m_frame_graph = new fg::FrameGraph();
		tasks::AddGenerateCubemapTask(*m_frame_graph);
		tasks::AddGenerateIrradianceMapTask(*m_frame_graph);
		tasks::AddGenerateEnvironmentMapTask(*m_frame_graph);
		tasks::AddGenerateBRDFLutTask(*m_frame_graph);
		tasks::AddDeferredMainTask(*m_frame_graph);
		tasks::AddDeferredCompositionTask(*m_frame_graph);
		tasks::AddPostProcessingTask<tasks::DeferredCompositionData>(*m_frame_graph);
		tasks::AddCopyToBackBufferTask<tasks::PostProcessingData>(*m_frame_graph);
		tasks::AddImGuiTask(*m_frame_graph, [this]() { editor.Render(); });

		m_renderer = new Renderer();
		m_renderer->Init(this);

		auto model_pool = m_renderer->GetModelPool();
		auto texture_pool = m_renderer->GetTexturePool();
		m_material_pool = m_renderer->GetMaterialPool();
		m_robot_model_handle = model_pool->LoadWithMaterials<Vertex>("robot/scene.gltf", m_material_pool, texture_pool, true);
		auto sphere_model_handle = model_pool->LoadWithMaterials<Vertex>("sphere.fbx", m_material_pool, texture_pool, true);

		float num_spheres_x = 11;
		float num_spheres_y = 11;
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

		m_frame_graph->Setup(m_renderer);

		m_renderer->Upload();

		m_scene_graph = new sg::SceneGraph(m_renderer);

		m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
		sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0, 0, 2.5));
		sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(0, -90._deg, 0));

		m_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_robot_model_handle);
		sg::helper::SetPosition(m_scene_graph, m_node, glm::vec3(-0.75, -1, 0));
		sg::helper::SetScale(m_scene_graph, m_node, glm::vec3(0.01, 0.01, 0.01));
		sg::helper::SetRotation(m_scene_graph, m_node, glm::vec3(-90._deg, 0, 0));

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
		{
			auto node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT);
			sg::helper::SetPosition(m_scene_graph, node, glm::vec3(4.805, -2.800, -1.200));
		}

		m_last = std::chrono::high_resolution_clock::now();
	}

	void Loop() final
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto diff = now - m_last; 
		m_delta = (float)diff.count() / 1000000000.f; // milliseconds
		m_last = now;

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

		float speed = m_move_speed * m_delta;
		auto forward_right = sg::helper::GetForwardRight(m_scene_graph, m_camera_node);
		sg::helper::Translate(m_scene_graph, m_camera_node, (m_z_axis.z * speed) * forward_right.first);
		sg::helper::Translate(m_scene_graph, m_camera_node, (m_z_axis.y * speed) * glm::vec3(0, 1, 0));
		sg::helper::Translate(m_scene_graph, m_camera_node, (m_z_axis.x * speed) * forward_right.second);
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		m_renderer->Resize(width, height);
		m_frame_graph->Resize(width, height);
	}

	void HandleControllerInput()
	{
		GLFWgamepadstate state;
		if (GetGamepad(GLFW_JOYSTICK_1, &state))
		{
			float dead_zone = 0.1;
			if (!m_rmb)
			{
				m_z_axis = { 0, 0, 0 };
			}

			// rotation
			{
				float x_axis = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
				float y_axis = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * -1;
				float sensitivity = m_controller_sensitivity * m_delta;
				if (m_flip_controller_y)
				{
					y_axis = y_axis * -1;
				}

				if (x_axis > dead_zone || y_axis > dead_zone || x_axis < -dead_zone || y_axis < -dead_zone)
				{
					sg::helper::Rotate(m_scene_graph, m_camera_node, glm::vec3(y_axis * sensitivity, x_axis * sensitivity, 0));
				}
			}
			// translation
			{
				float x_axis = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
				float z_axis = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * -1;
				float y_plus_axis = (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1) / 2;
				float y_minus_axis = (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1) / 2;

				if (x_axis > dead_zone || z_axis > dead_zone || x_axis < -dead_zone || z_axis < -dead_zone)
				{
					m_z_axis.x = x_axis;
					m_z_axis.z = z_axis;
				}

				if (y_plus_axis > dead_zone || y_plus_axis < -dead_zone)
				{
					m_z_axis.y += y_plus_axis;
				}
				if (y_minus_axis > dead_zone || y_minus_axis < -dead_zone)
				{
					m_z_axis.y -= y_minus_axis;
				}

				m_z_axis = glm::clamp(m_z_axis, glm::vec3(-1), glm::vec3(1));
			}

		}
	}

	void MousePosCallback(float x, float y) final
	{
		if (!m_rmb) return;

		glm::vec2 center = glm::vec2(GetWidth() / 2.f, GetHeight() / 2.f);

		float x_movement = x - center.x;
		float y_movement = center.y - y;
		float sensitivity = m_mouse_sensitivity * m_delta;

		sg::helper::Rotate(m_scene_graph, m_camera_node, glm::vec3(y_movement * sensitivity, x_movement * sensitivity, 0));

		SetMousePos(GetWidth() / 2.f, GetHeight() / 2.f);
	}

	void MouseButtonCallback(int key, int action) final
	{
		if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{
			m_rmb = true;
			glm::vec2 center = glm::vec2(GetWidth() / 2.f, GetHeight() / 2.f);

			SetMousePos(center.x, center.y);
			SetMouseVisibility(false);
		}
		else if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		{
			m_rmb = false;
			SetMouseVisibility(true);

			m_z_axis = glm::vec3(0);
		}

	}

	void KeyCallback(int key, int action) final
	{
		if (!ImGui::GetIO().WantCaptureKeyboard)
		{
			if (key == GLFW_KEY_W)
			{
				m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
			}
			else if (key == GLFW_KEY_E)
			{
				m_gizmo_operation = ImGuizmo::OPERATION::ROTATE;
			}
			else if (key == GLFW_KEY_R)
			{
				m_gizmo_operation = ImGuizmo::OPERATION::SCALE;
			}
		}

		if (!m_rmb) return;

		float axis_mod = 0;
		if (action == GLFW_PRESS)
		{
			axis_mod = 1;
		}
		else if (action == GLFW_RELEASE)
		{
			axis_mod = -1;
		}

		if (key == GLFW_KEY_W)
		{
			m_z_axis.z += axis_mod;	
		}
		else if (key == GLFW_KEY_S)
		{
			m_z_axis.z -= axis_mod;
		}
		else if (key == GLFW_KEY_A)
		{
			m_z_axis.x -= axis_mod;
		}
		else if (key == GLFW_KEY_D)
		{
			m_z_axis.x += axis_mod;
		}
		else if (key == GLFW_KEY_SPACE)
		{
			m_z_axis.y += axis_mod;
		}
		else if (key == GLFW_KEY_LEFT_CONTROL)
		{
			m_z_axis.y -= axis_mod;
		}

		m_z_axis = glm::clamp(m_z_axis, glm::vec3(-1), glm::vec3(1));
	}

	Editor editor;
	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;
	sg::SceneGraph* m_scene_graph;

	sg::NodeHandle m_node;
	sg::NodeHandle m_camera_node;

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

	// Camera Movement
	float m_move_speed = 5;
	float m_mouse_sensitivity = 0.4;
	float m_controller_sensitivity = 2.5;
	glm::vec3 m_z_axis = glm::vec3(0);
	bool m_rmb = false;
	bool m_flip_controller_y = false;

	ImGuizmo::OPERATION m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

	// ImGui
	std::chrono::time_point<std::chrono::high_resolution_clock> m_last;
	std::vector<float> m_frame_rates;
	int m_max_frame_rates = 1000;
	float m_min_frame_rate = 0;
	float m_max_frame_rate = 1;
	std::optional<sg::NodeHandle> m_selected_node;
	ImGuiTextFilter m_outliner_filter;
};


ENTRY(Demo, 1280, 720)
