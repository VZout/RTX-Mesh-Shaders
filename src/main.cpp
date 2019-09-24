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
		editor.RegisterCategory("Stats");
		editor.RegisterCategory("Help");

		// Actions
		editor.RegisterAction("Quit", "File", [&](){ Close(); });
		editor.RegisterAction("Contribute", "Help", [&](){ OpenURL("https://github.com/VZout/RTX-Mesh-Shaders"); });
		editor.RegisterAction("Report Issue", "Help", [&](){ OpenURL("https://github.com/VZout/RTX-Mesh-Shaders/issues"); });

		// Windows
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
		auto model_handle = model_pool->LoadWithMaterials<Vertex>("robot/scene.gltf", material_pool, texture_pool, true);

		m_frame_graph->Setup(m_renderer);

		m_renderer->Upload();

		m_scene_graph = new sg::SceneGraph();
		m_node = m_scene_graph->CreateNode();
		m_scene_graph->PromoteNode<sg::MeshComponent>(m_node, model_handle);
		sg::helper::SetPosition(m_scene_graph, m_node, glm::vec3(0, -1, 0));
		sg::helper::SetScale(m_scene_graph, m_node, glm::vec3(0.01, 0.01, 0.01));

		m_start = std::chrono::high_resolution_clock::now();
	}

	void Loop() final
	{
		auto diff = std::chrono::high_resolution_clock::now() - m_start;
		float t = diff.count();

		sg::helper::SetRotation(m_scene_graph, m_node, glm::vec3(-90._deg, glm::radians(t * 0.0000001f), 0));

		m_scene_graph->Update();
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
		m_renderer->Resize(width, height);
		m_frame_graph->Resize(width, height);
	}

	Editor editor;
	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;
	sg::SceneGraph* m_scene_graph;

	sg::NodeHandle m_node;

	// ImGui
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	std::vector<float> m_frame_rates;
	int m_max_frame_rates = 1000;
	float m_min_frame_rate;
	float m_max_frame_rate;
};


ENTRY(Demo, 1280, 720)
