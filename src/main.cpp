/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include <chrono>

#include "frame_graph/frame_graph.hpp"
#include "scene_graph/scene_graph.hpp"
#include "application.hpp"
#include "renderer.hpp"
#include "render_tasks/vulkan_tasks.hpp"

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
	void Interface()
	{
		ImGui::Begin("Performance");

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

		ImGui::PlotLines("Framerate", m_frame_rates.data(), m_frame_rates.size(), 0, nullptr, m_min_frame_rate, m_max_frame_rate, ImVec2(ImGui::GetContentRegionAvail()));

		ImGui::End();
	}

	void Init() final
	{
		m_frame_graph = new fg::FrameGraph();
		tasks::AddDeferredMainTask(*m_frame_graph);
		tasks::AddImGuiTask(*m_frame_graph, [this]() { Interface(); });

		m_renderer = new Renderer();
		m_renderer->Init(this);
		m_frame_graph->Setup(m_renderer);
		m_renderer->Upload();

		m_scene_graph = new sg::SceneGraph();
		m_node = m_scene_graph->CreateNode();
		m_scene_graph->PromoteNode<sg::MeshComponent>(m_node);
		sg::helper::SetScale(m_scene_graph, m_node, glm::vec3(0.01, 0.01, 0.01));

		m_start = std::chrono::high_resolution_clock::now();
	}

	void Loop() final
	{
		auto diff = std::chrono::high_resolution_clock::now() - m_start;
		float t = diff.count();

		sg::helper::SetRotation(m_scene_graph, m_node, glm::vec3(glm::radians(-90.f), glm::radians(t * 0.0000001f), 0));

		m_scene_graph->Update();
		m_renderer->Render(*m_scene_graph, *m_frame_graph);

		auto append_graph_list = [](auto& list, auto time, auto max, auto& lowest, auto& highest)
		{
			lowest = time < lowest ? time : lowest;
			highest = time > highest ? time : highest;

			if (list.size() > max) //Max seconds to show
			{
				for (size_t i = 1; i < list.size(); i++)
				{
					list[i-1] = list[i];
				}
				list[list.size() - 1] = time;
			}
			else
			{
				list.push_back(time);
			}
		};

		append_graph_list(m_frame_rates, ImGui::GetIO().Framerate, m_max_frame_rates, m_min_frame_rate, m_max_frame_rate);
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		m_renderer->Resize(width, height);
	}

	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;
	sg::SceneGraph* m_scene_graph;

	sg::NodeHandle m_node;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	std::vector<float> m_frame_rates;
	int m_max_frame_rates = 1000;
	float m_min_frame_rate;
	float m_max_frame_rate;
};


ENTRY(Demo, 1280, 720)