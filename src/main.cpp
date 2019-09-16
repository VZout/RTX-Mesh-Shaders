/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

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
	void Init() final
	{
		m_frame_graph = new fg::FrameGraph();
		tasks::AddDeferredMainTask(*m_frame_graph);
		tasks::AddImGuiTask(*m_frame_graph);

		m_renderer = new Renderer();
		m_renderer->Init(this);
		m_frame_graph->Setup(m_renderer);
		m_renderer->Upload();


		m_scene_graph = new sg::SceneGraph();
		auto node = m_scene_graph->CreateNode();
		m_scene_graph->PromoteNode<sg::MeshComponent>(node);
	}

	void Loop() final
	{
		m_renderer->Render(*m_scene_graph, *m_frame_graph);
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		m_renderer->Resize(width, height);
	}

	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;
	sg::SceneGraph* m_scene_graph;
};


ENTRY(Demo, 1280, 720)