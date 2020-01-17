/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "market_scene.hpp"

#include <util/user_literals.hpp>
#include <vertex.hpp>
#include <future>
#include <thread>
#include <random>

MarketScene::MarketScene() :
	Scene("Spaceship Scene")
{

}

void MarketScene::LoadResources(std::optional<std::reference_wrapper<util::Progress>> progress)
{
	if (progress) MAKE_CHILD_PROGRESS((*progress).get(), 1);

	if (progress) PROGRESS((*progress).get(), "Loading Market Model");
	m_market_model = m_model_pool->LoadWithMaterials<Vertex>("market/scene.gltf", m_material_pool, m_texture_pool, false);

	if (progress) POP_CHILD_PROGRESS((*progress).get());
}

void MarketScene::BuildScene(std::optional<std::reference_wrapper<util::Progress>> progress)
{
	if (progress) MAKE_CHILD_PROGRESS((*progress).get(), 1);

	if (progress) PROGRESS((*progress).get(), "Creating Camera and Scene")

	// Create Camera
	m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
	sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(-6.345, 4.804, -0.584));
	sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(-6.468_deg, 25.249_deg, 0));
	sg::helper::SetLensDiameter(m_scene_graph, m_camera_node, 0.1f);
	sg::helper::SetFieldOfView(m_scene_graph, m_camera_node, 40.f);
	sg::helper::SetFocalDistance(m_scene_graph, m_camera_node, 6.0f);

	auto object = m_scene_graph->CreateNode<sg::MeshComponent>(m_market_model);
	sg::helper::SetPosition(m_scene_graph, object, glm::vec3(0, 0, 0));
	sg::helper::SetScale(m_scene_graph, object, glm::vec3(0.1));
	//sg::helper::SetRotation(m_scene_graph, object, glm::vec3(0, 0_deg, 0g));

	auto light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(400, 300, 200));
	sg::helper::SetPosition(m_scene_graph, light_node, { -8.359, 6.315, 2.043 });
	sg::helper::SetPhysicalSize(m_scene_graph, light_node, 0.1);

	auto fire_light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(30, 5, 0));
	sg::helper::SetPosition(m_scene_graph, fire_light_node, { 6.409, 3.740, 5.781 });
	sg::helper::SetPhysicalSize(m_scene_graph, fire_light_node, 0.05);

	if (progress) POP_CHILD_PROGRESS((*progress).get());
}

void MarketScene::Update_Impl(float delta, float time)
{
}
