/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include <chrono>

#include <frame_graph/frame_graph.hpp>
#include <application.hpp>
#include <util/version.hpp>
#include <util/user_literals.hpp>
#include <render_tasks/vulkan_tasks.hpp>

#define NUM_FRAMES_TO_RENDER 6

class PBRTest : public Application
{
public:
	PBRTest()
		: Application("PBR Test"),
		  m_renderer(nullptr)
	{
	}

	~PBRTest() final
	{
		delete m_frame_graph;
		delete m_scene_graph;
		delete m_renderer;
	}

protected:

	void Init() final
	{
		m_frame_graph = new fg::FrameGraph();
		tasks::AddGenerateCubemapTask(*m_frame_graph);
		tasks::AddGenerateIrradianceMapTask(*m_frame_graph);
		tasks::AddGenerateEnvironmentMapTask(*m_frame_graph);
		tasks::AddGenerateBRDFLutTask(*m_frame_graph);
		tasks::AddDeferredMainTask(*m_frame_graph);
		tasks::AddDeferredCompositionTask(*m_frame_graph);
		tasks::AddPostProcessingTask<tasks::DeferredCompositionData>(*m_frame_graph);
		tasks::AddCopyToBackBufferTask<tasks::PostProcessingData>(*m_frame_graph);

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

		auto camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
		sg::helper::SetPosition(m_scene_graph, camera_node, glm::vec3(0, 0, -2.5));

		auto node = m_scene_graph->CreateNode<sg::MeshComponent>(robot_model_handle);
		sg::helper::SetPosition(m_scene_graph, node, glm::vec3(-0.75, -1, 0));
		sg::helper::SetScale(m_scene_graph, node, glm::vec3(0.01, 0.01, 0.01));

		// second node
		{
			auto battery_node = m_scene_graph->CreateNode<sg::MeshComponent>(battery_model_handle);
			sg::helper::SetPosition(m_scene_graph, battery_node, glm::vec3(0.75, -0.65, 0));
			sg::helper::SetScale(m_scene_graph, battery_node, glm::vec3(0.01, 0.01, 0.01));
			sg::helper::SetRotation(m_scene_graph, battery_node, glm::vec3(glm::radians(-90.f), glm::radians(40.f), 0));
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
	}

	void Loop() final
	{
		if (m_rendered_frames >= NUM_FRAMES_TO_RENDER)
		{
			Close();
		}

		m_scene_graph->Update(m_renderer->GetFrameIdx());
		m_renderer->Render(*m_scene_graph, *m_frame_graph);
		m_rendered_frames++;
	}

	void ResizeCallback(std::uint32_t width, std::uint32_t height) final
	{
		m_renderer->Resize(width, height);
		m_frame_graph->Resize(width, height);
	}

	Renderer* m_renderer;
	fg::FrameGraph* m_frame_graph;
	sg::SceneGraph* m_scene_graph;

	std::uint32_t m_rendered_frames = 0;
};

ENTRY(PBRTest, 1280, 720)