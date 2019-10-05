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

	auto model_pool = renderer->GetModelPool();
	auto texture_pool = renderer->GetTexturePool();
	auto material_pool = renderer->GetMaterialPool();
	auto robot_model_handle = model_pool->LoadWithMaterials<Vertex>("robot/scene.gltf", material_pool, texture_pool, true);

	auto sg = new sg::SceneGraph();
	sg->SetPOConstantBufferPool(renderer->CreateConstantBufferPool(1));
	sg->SetCameraConstantBufferPool(renderer->CreateConstantBufferPool(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT));
	sg->SetLightConstantBufferPool(renderer->CreateConstantBufferPool(3, VK_SHADER_STAGE_COMPUTE_BIT));

	for (std::uint32_t i = 0; i < num_mesh_nodes; i++)
	{
		sg->CreateNode<sg::MeshComponent>(robot_model_handle);
	}

	for (auto _ : state)
	{
		sg->Update(0);
		sg->Update(1);
		sg->Update(2);
	}

	app->Close();

	delete sg;
	delete renderer;
	delete app;
}

BENCHMARK(BM_SceneGraphMeshNode);
BENCHMARK_MAIN();