/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include <chrono>

#include "frame_graph/frame_graph.hpp"
#include "application.hpp"
#include "editor.hpp"
#include "util/version.hpp"
#include "util/user_literals.hpp"
#include "render_tasks/vulkan_tasks.hpp"

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
	void SetupEditor()
	{
		// Categories
		editor.RegisterCategory("File");
		editor.RegisterCategory("Scene Graph");
		editor.RegisterCategory("Stats");
		editor.RegisterCategory("Help");

		// Actions
		editor.RegisterAction("Quit", "File", [&](){ Close(); });
		editor.RegisterAction("Contribute", "Help", [&](){ OpenURL("https://github.com/VZout/RTX-Mesh-Shaders"); });
		editor.RegisterAction("Report Issue", "Help", [&](){ OpenURL("https://github.com/VZout/RTX-Mesh-Shaders/issues"); });

		// Windows
		editor.RegisterWindow("World Outliner", "Scene Graph", [&]()
		{
			ImVec2 size = ImGui::GetContentRegionAvail();
			size.y -= ImGui::GetItemsLineHeightWithSpacing();
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
						name_prefix = "Mesh Node";
					}
					else if (node.m_camera_component > -1)
					{
						name_prefix = "Camera Node";
					}
					else if (node.m_light_component > -1)
					{
						name_prefix = "Light Node";
					}

					auto node_name = name_prefix + " (" + std::to_string(i) + ")";

					bool pressed = ImGui::Selectable(node_name.c_str(), m_selected_node == handle);
					if (pressed)
					{
						m_selected_node = handle;
					}
				}
				}
				ImGui::ListBoxFooter();
		});

		editor.RegisterWindow("Inspector", "Scene Graph", [&]()
		{
			if (!m_selected_node.has_value()) return;

			auto node = m_scene_graph->GetNode(m_selected_node.value());

			if (node.m_transform_component > -1)
			{
				ImGui::DragFloat3("Position", &m_scene_graph->m_positions[node.m_transform_component].m_value[0], 0.1f);

				auto quaternion = m_scene_graph->m_rotations[node.m_transform_component].m_value;
				auto euler = glm::degrees(glm::eulerAngles(quaternion));
				ImGui::DragFloat3("Rotation", &euler[0], 0.1f);
				m_scene_graph->m_rotations[node.m_transform_component].m_value = glm::quat(glm::radians(euler));

				if (node.m_camera_component == -1 && node.m_light_component == -1)
				{
					ImGui::DragFloat3("Scale", &m_scene_graph->m_scales[node.m_transform_component].m_value[0], 0.01f);
				}
			}

			if (node.m_light_component > -1)
			{
				ImGui::Separator();
				ImGui::DragFloat3("Color", &m_scene_graph->m_colors[node.m_light_component].m_value[0], 0.1f);
			}

			m_scene_graph->m_requires_update[node.m_transform_component] = true;
		});

		editor.RegisterWindow("Performance", "Stats", [&]()
		{
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 100);
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::NextColumn();
			ImGui::InputInt("Max Samples", &m_max_frame_rates);
			ImGui::SameLine();
			if (ImGui::Button("Reset Scale"))
			{
			  m_max_frame_rate = std::numeric_limits<float>::min();
			  m_min_frame_rate = std::numeric_limits<float>::max();
			}
			ImGui::Columns(1);

			ImGui::PlotLines("Framerate", m_frame_rates.data(), m_frame_rates.size(), 0, nullptr, m_min_frame_rate,
			               m_max_frame_rate, ImVec2(ImGui::GetContentRegionAvail()));
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
		tasks::AddDeferredMainTask(*m_frame_graph);
		tasks::AddDeferredCompositionTask(*m_frame_graph);
		tasks::AddPostProcessingTask<tasks::DeferredCompositionData>(*m_frame_graph);
		tasks::AddCopyToBackBufferTask<tasks::PostProcessingData>(*m_frame_graph);
		tasks::AddImGuiTask(*m_frame_graph, [this]() { editor.Render(); });

		m_renderer = new Renderer();
		m_renderer->Init(this);

		auto model_pool = m_renderer->GetModelPool();
		auto texture_pool = m_renderer->GetTexturePool();
		auto material_pool = m_renderer->GetMaterialPool();
		auto robot_model_handle = model_pool->LoadWithMaterials<Vertex>("robot/scene.gltf", material_pool, texture_pool, true);
		auto battery_model_handle = model_pool->LoadWithMaterials<Vertex>("scene.gltf", material_pool, texture_pool, true);

		m_frame_graph->Setup(m_renderer);

		m_renderer->Upload();

		m_scene_graph = new sg::SceneGraph();
		m_scene_graph->SetPOConstantBufferPool(m_renderer->CreateConstantBufferPool(1));
		m_scene_graph->SetCameraConstantBufferPool(m_renderer->CreateConstantBufferPool(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT));
		m_scene_graph->SetLightConstantBufferPool(m_renderer->CreateConstantBufferPool(3, VK_SHADER_STAGE_COMPUTE_BIT));

		m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
		sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0, 0, -2.5));

		m_node = m_scene_graph->CreateNode<sg::MeshComponent>(robot_model_handle);
		sg::helper::SetPosition(m_scene_graph, m_node, glm::vec3(-0.75, -1, 0));
		sg::helper::SetScale(m_scene_graph, m_node, glm::vec3(0.01, 0.01, 0.01));

		// second node
		{
			auto node = m_scene_graph->CreateNode<sg::MeshComponent>(battery_model_handle);
			sg::helper::SetPosition(m_scene_graph, node, glm::vec3(0.75, -0.65, 0));
			sg::helper::SetScale(m_scene_graph, node, glm::vec3(0.01, 0.01, 0.01));
			sg::helper::SetRotation(m_scene_graph, node, glm::vec3(glm::radians(-90.f), glm::radians(40.f), 0));
		}

		// light node
		{
			auto node = m_scene_graph->CreateNode<sg::LightComponent>();
			sg::helper::SetPosition(m_scene_graph, node, glm::vec3(0.f, 0.f, -4));
		}
		// light node
		{
			auto node = m_scene_graph->CreateNode<sg::LightComponent>(glm::vec3{ 1, 0, 0 });
			sg::helper::SetPosition(m_scene_graph, node, glm::vec3(1.f, 0.f, -0.5));
		}
		// light node
		{
			auto node = m_scene_graph->CreateNode<sg::LightComponent>(glm::vec3{ 0, 0, 1 });
			sg::helper::SetPosition(m_scene_graph, node, glm::vec3(-1.f, 0.f, -0.5));
		}

		m_start = std::chrono::high_resolution_clock::now();
	}

	void Loop() final
	{
		auto diff = std::chrono::high_resolution_clock::now() - m_start;
		float t = diff.count();

		sg::helper::SetRotation(m_scene_graph, m_node, glm::vec3(-90._deg, glm::radians(t * 0.0000001f), 0));

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
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		//m_renderer->Resize(width, height);
		//m_frame_graph->Resize(width, height);
	}

	Editor editor;
	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;
	sg::SceneGraph* m_scene_graph;

	sg::NodeHandle m_node;
	sg::NodeHandle m_camera_node;

	// ImGui
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	std::vector<float> m_frame_rates;
	int m_max_frame_rates = 1000;
	float m_min_frame_rate;
	float m_max_frame_rate;
	std::optional<sg::NodeHandle> m_selected_node;
};


ENTRY(Demo, 1280, 720)
