/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "scene_graph.hpp"
#include "application.hpp"
#include "renderer.hpp"

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
		delete m_renderer;
	}

protected:
	void Init() final
	{
		m_renderer = new Renderer();
		m_renderer->Init(this);

		auto sg = new SceneGraph();
		auto node = sg->CreateNode();
		sg->PromoteNode<MeshComponent>(node);
		delete sg;
	}

	void Loop() final
	{
		m_renderer->Render();
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		m_renderer->Resize(width, height);
	}

	Renderer* m_renderer;
};


ENTRY(Demo, 1280, 720)