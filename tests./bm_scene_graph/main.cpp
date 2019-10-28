#include <benchmark/benchmark.h>

#include <renderer.hpp>
#include <scene_graph/scene_graph.hpp>
#include <application.hpp>
#include <vertex.hpp>

static std::uint32_t num_mesh_nodes = 100;

class EmptyApp : public Application
{
public:
	EmptyApp()
		: Application("PBR Test")
	{
	}

protected:
	void Init() final {	}
	void Loop() final {	}

};

static void BM_SceneGraphMeshNode(benchmark::State& state) {
	auto app = new EmptyApp();
	app->Create(100, 100);

	auto renderer = new Renderer();
	renderer->Init(app);

	auto sg = new sg::SceneGraph(renderer);

	std::vector<sg::NodeHandle> nodes(num_mesh_nodes);
	for (auto& node : nodes)
	{
		node = sg->CreateNode<sg::TransformComponent>();
	}

	for (auto _ : state)
	{
		for (auto& node : nodes)
		{
			sg::helper::SetPosition(sg, node, { 0, 0, 0 });
		}
		sg->Update(0);
	}

	app->Close();

	delete sg;
	delete renderer;
	delete app;
}

BENCHMARK(BM_SceneGraphMeshNode);
BENCHMARK_MAIN();