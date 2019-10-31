/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

void SetupEditor()
{
	// Categories
	editor.RegisterCategory("File", reinterpret_cast<const char*>(ICON_FA_FILE));
	editor.RegisterCategory("Scene Graph", reinterpret_cast<const char*>(ICON_FA_PROJECT_DIAGRAM), "Scenes");
	editor.RegisterCategory("Stats", reinterpret_cast<const char*>(ICON_FA_CHART_BAR));
	editor.RegisterCategory("Debug", reinterpret_cast<const char*>(ICON_FA_BUG));
	editor.RegisterCategory("Help", reinterpret_cast<const char*>(ICON_FA_INFO_CIRCLE));

	// Actions
	editor.RegisterAction("Quit", "File", [&]() { Close(); }, reinterpret_cast<const char*>(ICON_FA_POWER_OFF));
	editor.RegisterAction("Save ImGui Settings", "File", [&]() { ImGui::SaveIniSettingsToDisk(settings::imgui_ini_filename); }, reinterpret_cast<const char*>(ICON_FA_SAVE));
	editor.RegisterAction("Contribute", "Help", [&]() { OpenURL("https://github.com/VZout/RTX-Mesh-Shaders"); }, reinterpret_cast<const char*>(ICON_FA_HANDS_HELPING));
	editor.RegisterAction("Report Issue", "Help", [&]() { OpenURL("https://github.com/VZout/RTX-Mesh-Shaders/issues"); }, reinterpret_cast<const char*>(ICON_FA_BUG));
	editor.RegisterAction("Key Bindings", "Help", [&]() { editor.OpenModal("Key Bindings"); });

	editor.RegisterAction("Load Spheres Scene", "Scene Graph", [&]() { SwitchScene<SpheresScene>(); }, std::nullopt, "Scenes");
	editor.RegisterAction("Load Subsurface Scene", "Scene Graph", [&]() { SwitchScene<SubsurfaceScene>(); }, std::nullopt, "Scenes");

	// Windows
	editor.RegisterWindow("World Outliner", "Scene Graph", [&]()
		{
			auto scene_graph = m_scene->GetSceneGraph();

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
				const auto& node_handles = scene_graph->GetNodeHandles();
				for (std::size_t i = 0; i < node_handles.size(); i++)
				{
					auto handle = node_handles[i];
					auto node = scene_graph->GetNode(handle);

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
						switch (scene_graph->m_light_types[node.m_light_component])
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
				auto node = scene_graph->GetNode(m_selected_node.value());
				bool enable_gizmo = true;

				if (node.m_light_component > -1)
				{
					if (m_gizmo_operation == ImGuizmo::OPERATION::SCALE)
					{
						enable_gizmo = false;
					}

					switch (scene_graph->m_light_types[node.m_light_component])
					{
					case cb::LightType::POINT:
						if (m_gizmo_operation == ImGuizmo::OPERATION::ROTATE)
						{
							enable_gizmo = false;
						}
					break;
					case cb::LightType::DIRECTIONAL:
						if (m_gizmo_operation == ImGuizmo::OPERATION::TRANSLATE)
						{
							enable_gizmo = false;
						}
					break;
					}
				}

				ImGuizmo::Enable(enable_gizmo);

				if (node.m_camera_component == -1)
				{
					ImGui_ManipulateNode(node, m_gizmo_operation);
				}
			}
		}, true, reinterpret_cast<const char*>(ICON_FA_GLOBE_EUROPE));

	/*editor.RegisterWindow("Temp Material Settings", "Scene Graph", [&]()
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

		}, false, reinterpret_cast<const char*>(ICON_FA_PALETTE));*/

	editor.RegisterWindow("Inspector", "Scene Graph", [&]()
		{
			if (!m_selected_node.has_value()) return;

			auto scene_graph = m_scene->GetSceneGraph();

			auto node = scene_graph->GetNode(m_selected_node.value());

			if (node.m_transform_component > -1)
			{
				if (node.m_light_component == -1 || scene_graph->m_light_types[node.m_light_component] != cb::LightType::DIRECTIONAL)
				{
					ImGui::DragFloat3("Position", &scene_graph->m_positions[node.m_transform_component].m_value[0], 0.1f);
				}

				if (node.m_light_component == -1 || scene_graph->m_light_types[node.m_light_component] != cb::LightType::POINT)
				{
					auto euler = glm::degrees(scene_graph->m_rotations[node.m_transform_component].m_value);
					ImGui::DragFloat3("Rotation", &euler[0], 0.1f);
					scene_graph->m_rotations[node.m_transform_component].m_value = glm::radians(euler);
				}

				if (node.m_camera_component == -1 && node.m_light_component == -1)
				{
					constexpr float min = 0.0000000000001f;
					constexpr float max = std::numeric_limits<float>::max();

					auto& scale = scene_graph->m_scales[node.m_transform_component].m_value;
					ImGui::DragFloat3("Scale", &scale[0], 0.01f, min, max);
					scale = glm::max(scale, min);
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
				int selected_type = (int)scene_graph->m_light_types[node.m_light_component].m_value;
				ImGui::Combo("Type", &selected_type, types, 3);
				scene_graph->m_light_types[node.m_light_component].m_value = (cb::LightType)selected_type;

				ImGui::DragFloat3("Color", &scene_graph->m_colors[node.m_light_component].m_value[0], 0.1f);

				if (scene_graph->m_light_types[node.m_light_component] == cb::LightType::POINT)
				{
					ImGui::DragFloat("Radius", &scene_graph->m_radius[node.m_light_component].m_value, 0.05f);
				}

				if (scene_graph->m_light_types[node.m_light_component] == cb::LightType::SPOT)
				{
					auto inner = glm::degrees(scene_graph->m_light_angles[node.m_light_component].m_value.first);
					auto outer = glm::degrees(scene_graph->m_light_angles[node.m_light_component].m_value.second);

					ImGui::DragFloat("Inner Angle", &inner);
					ImGui::DragFloat("Outer Angle", &outer);

					scene_graph->m_light_angles[node.m_light_component].m_value.first = glm::radians(inner);
					scene_graph->m_light_angles[node.m_light_component].m_value.second = glm::radians(outer);
				}
			}

			scene_graph->m_requires_update[node.m_transform_component] = true;
		}, true, reinterpret_cast<const char*>(ICON_FA_EYE));

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
		}, false, reinterpret_cast<const char*>(ICON_FA_CHART_AREA));

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
		}, true, reinterpret_cast<const char*>(ICON_FA_MICROCHIP));

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
		}, false, reinterpret_cast<const char*>(ICON_FA_MEMORY));

	m_viewport_has_focus = false;
	editor.RegisterWindow("Viewport", "Debug", [&]()
		{
			m_viewport_has_focus = ImGui::IsWindowFocused();

			if (m_fps_camera.IsControlled())
			{
				ImGui::SetWindowFocus();
			}

			ImVec2 size = ImGui::GetContentRegionAvail();
			m_viewport_pos = ImGui::GetCursorScreenPos();

			// If the size changed...
			if (size.x != m_viewport_size.x || size.y != m_viewport_size.y)
			{
				auto scene_graph = m_scene->GetSceneGraph();
				auto camera_node = m_scene->GetCameraNodeHandle();
				m_viewport_size = size;
				sg::helper::SetAspectRatio(scene_graph, camera_node, (float)size.x / (float)size.y);
			}

			ImGui::Image(editor.GetTexture(), size);
		}, true, reinterpret_cast<const char*>(ICON_FA_GAMEPAD));

	editor.RegisterWindow("Input Settings", "Debug", [&]()
		{
			auto& settings = m_fps_camera.GetSettings();

			ImGui::DragFloat("Movement Speed", &settings.m_move_speed, 1, 0, 100);
			ImGui::DragFloat("Mouse Sensitivity", &settings.m_mouse_sensitivity, 1, 0, 100);
			ImGui::DragFloat("Controller Sensitivity", &settings.m_controller_sensitivity, 1, 0, 100);
			ImGui::ToggleButton("Inverted Controller Y", &settings.m_flip_controller_y);

			if (ImGui::Button("Keyboard Bindings"))
			{
				editor.OpenModal("Key Bindings");
			}

		}, true, reinterpret_cast<const char*>(ICON_FA_KEYBOARD));

	editor.RegisterWindow("About", "Help", [&]()
		{
			ImGui::Text("Turing Mesh Shading");
			constexpr auto version = util::GetVersion();
			ImGui::InfoText("Version", util::VersionToString(version), false);
			ImGui::Separator();
			ImGui::Text("Copyright 2019 Viktor Zoutman");
			if (ImGui::Button("License")) OpenURL("https://github.com/VZout/RTX-Mesh-Shaders/blob/master/LICENSE");
			ImGui::SameLine();
			if (ImGui::Button("Portfolio")) OpenURL("http://www.vzout.com/");
		}, false, reinterpret_cast<const char*>(ICON_FA_ADDRESS_CARD));

	// Modals
	editor.RegisterModal("Key Bindings", [&]()
	{
		ImGui::InfoText("Move Forward", "RMB + W");
		ImGui::InfoText("Move Backward", "RMB + S");
		ImGui::InfoText("Move Left", "RMB + A");
		ImGui::InfoText("Move Right", "RMB + D");
		ImGui::InfoText("Look Left/Right", "RMB + MouseX");
		ImGui::InfoText("Look Up/Down", "RMB + MouseY");
		ImGui::NewLine();
		ImGui::InfoText("Switch To Generic Pipeline", "1");
		ImGui::InfoText("Switch To Mesh Shading Pipeline", "2");
		ImGui::NewLine();
		ImGui::InfoText("Open Key bindings", "F1");
		ImGui::InfoText("Toggle Editor", "F3 / ESC");
		ImGui::InfoText("Toggle Fullscreen", "F11 / Alt + Enter");
		ImGui::InfoText("Close Application", "Alt + F4");
		ImGui::NewLine();
		ImGui::InfoText("Dock ImGui Window", "(hold) Shift");
		ImGui::InfoText("Cycle Through ImGui Windows", "Ctrl + Tab");
		ImGui::NewLine();
		ImGui::InfoText("Switch To Translate Gizmo", "W");
		ImGui::InfoText("Switch To Rotate Gizmo", "E");
		ImGui::InfoText("Switch To Scale Gizmo", "R");
		ImGui::NewLine();

		if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
	});
}
