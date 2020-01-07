/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "spaceship_scene.hpp"

#include <util/user_literals.hpp>
#include <vertex.hpp>
#include <future>
#include <thread>
#include <random>

SpaceshipScene::SpaceshipScene() :
	Scene("Spaceship Scene")
{

}

void SpaceshipScene::LoadResources(std::optional<std::reference_wrapper<util::Progress>> progress)
{
	if (progress) MAKE_CHILD_PROGRESS((*progress).get(), 4);

	if (progress) PROGRESS((*progress).get(), "Loading `forrest_ground_01_diff_4k`")

	auto image_loader = new STBImageLoader(); // TODO: Memory leak

	auto load_texture_func = [&](std::string path) { return image_loader->LoadFromDisc(path); };

	if (progress) PROGRESS((*progress).get(), "Loading Floor Model");
	m_plane_model = m_model_pool->LoadWithMaterials<Vertex>("plane.fbx", m_material_pool, m_texture_pool, false);
	if (progress) PROGRESS((*progress).get(), "Loading Spaceship Model");
	m_spaceship_model = m_model_pool->LoadWithMaterials<Vertex>("naboo/scene.gltf", m_material_pool, m_texture_pool, false);
	if (progress) PROGRESS((*progress).get(), "Loading Rock 0 Model");
	m_rock0_model = m_model_pool->LoadWithMaterials<Vertex>("rock0/scene.gltf", m_material_pool, m_texture_pool, false);

	if (progress) POP_CHILD_PROGRESS((*progress).get());
	int x = 0;
}

void SpaceshipScene::BuildScene(std::optional<std::reference_wrapper<util::Progress>> progress)
{
	if (progress) MAKE_CHILD_PROGRESS((*progress).get(), 1);

	if (progress) PROGRESS((*progress).get(), "Creating Camera, robot and floor")

	// Create Camera
	m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
	sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(-3.071, 3.25, 2.612));
	sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(-2.522_deg, -42.677_deg, 0));
	sg::helper::SetLensDiameter(m_scene_graph, m_camera_node, 0.0f);
	sg::helper::SetFieldOfView(m_scene_graph, m_camera_node, 35.f);
	sg::helper::SetFocalDistance(m_scene_graph, m_camera_node, 5.0f);

	// Create Spaceship
	auto object = m_scene_graph->CreateNode<sg::MeshComponent>(m_spaceship_model);
	sg::helper::SetPosition(m_scene_graph, object, glm::vec3(0, 2.871, -0.427));
	sg::helper::SetScale(m_scene_graph, object, glm::vec3(0.005));
	sg::helper::SetRotation(m_scene_graph, object, glm::vec3(0, 8.593_deg, 1.295_deg));

	// Create Rocks
	auto rock0 = m_scene_graph->CreateNode<sg::MeshComponent>(m_rock0_model);
	sg::helper::SetPosition(m_scene_graph, rock0, glm::vec3(0, -1.850, 0));
	sg::helper::SetScale(m_scene_graph, rock0, glm::vec3(5));
	sg::helper::SetRotation(m_scene_graph, rock0, glm::vec3(0, 0, 0));

	if (progress) POP_CHILD_PROGRESS((*progress).get());
}

void SpaceshipScene::Update_Impl(float delta, float time)
{
}
