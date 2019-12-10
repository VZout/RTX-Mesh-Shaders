/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "forrest_scene.hpp"

#include <util/user_literals.hpp>
#include <vertex.hpp>
#include <future>
#include <thread>
#include <random>

static const bool place_random_grass = true;
static const std::uint32_t num_grass_nodes = 150;
static const std::uint32_t num_tree_nodes = 100;
static const float scene_size = 10;

ForrestScene::ForrestScene() :
	Scene("Forrest Scene")
{

}

void ForrestScene::LoadResources()
{
	auto image_loader = new STBImageLoader(); // TODO: Memory leak

	auto load_texture_func = [&](std::string path) { return image_loader->LoadFromDisc(path); };

	auto albedo_f = std::async(std::launch::async, load_texture_func, "forrest_ground/forrest_ground_01_diff_4k.jpg");
	auto roughness_f = std::async(std::launch::async, load_texture_func, "forrest_ground/forrest_ground_01_rough_ao_rough_metallic.jpg");
	auto displacement_f = std::async(std::launch::async, load_texture_func, "forrest_ground/forrest_ground_01_disp_4k.jpg");
	auto normal_map_f = std::async(std::launch::async, load_texture_func, "forrest_ground/forrest_ground_01_nor_4k.jpg");

	ExtraMaterialData data_gass;
	data_gass.m_thickness_texture_paths = { "black.png", "black.png", "black.png", "black.png", "black.png" };

	ExtraMaterialData data_tree;
	data_tree.m_thickness_texture_paths = { "white.png", "black.png" };

	m_plane_model = m_model_pool->LoadWithMaterials<Vertex>("plane.fbx", m_material_pool, m_texture_pool, false);
	m_object_model = m_model_pool->LoadWithMaterials<Vertex>("robot/scene.gltf", m_material_pool, m_texture_pool, false);
	m_grass_model = m_model_pool->LoadWithMaterials<Vertex>("grass/scene.gltf", m_material_pool, m_texture_pool, false, data_gass);
	m_tree_model = m_model_pool->LoadWithMaterials<Vertex>("tree/scene.gltf", m_material_pool, m_texture_pool, false, data_tree);

	MaterialData mat = {};
	mat.m_base_reflectivity = 0.4f;
	mat.m_albedo_texture = *albedo_f.get();
	mat.m_roughness_texture = *roughness_f.get();
	mat.m_displacement_texture = *displacement_f.get();
	mat.m_normal_map_texture = *normal_map_f.get();
	mat.m_base_metallic = 0;
	mat.m_base_uv_scale = glm::vec2(5);

	m_plane_material_handle = m_material_pool->Load(mat, m_texture_pool);
}

void ForrestScene::BuildScene()
{
	// Create Camera
	m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
	sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0.5, 0.95, 2.6));
	sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(0, -90._deg, 0));
	sg::helper::SetLensDiameter(m_scene_graph, m_camera_node, 0.1f);
	sg::helper::SetFocalDistance(m_scene_graph, m_camera_node, 2.1f);

	// Create Object
	auto object = m_scene_graph->CreateNode<sg::MeshComponent>(m_object_model);
	sg::helper::SetPosition(m_scene_graph, object, glm::vec3(0, 0, 0));
	sg::helper::SetScale(m_scene_graph, object, glm::vec3(0.008));
	sg::helper::SetRotation(m_scene_graph, object, glm::vec3(-90._deg, 0, 0));

	auto plane_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_plane_model);
	sg::helper::SetMaterial(m_scene_graph, plane_node, { m_plane_material_handle });
	sg::helper::SetScale(m_scene_graph, plane_node, { scene_size * 2.f, 1, scene_size * 2.f });

	// Create Light
	auto light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(0, 1, 6));
	sg::helper::SetPosition(m_scene_graph, light_node, glm::vec3(0.048, 1.126, 1.167));
	sg::helper::SetRadius(m_scene_graph, light_node, 0);
	sg::helper::SetPhysicalSize(m_scene_graph, light_node, 0.1);

	// Create Light
	auto light_node2 = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(5, 1, 0));
	sg::helper::SetPosition(m_scene_graph, light_node2, glm::vec3(1.143, 1.362, 0.691));
	sg::helper::SetRadius(m_scene_graph, light_node2, 0);
	sg::helper::SetPhysicalSize(m_scene_graph, light_node2, 0.1);

	std::seed_seq seed{ 0, 0 };
	std::mt19937 gen(seed);
	std::uniform_real_distribution<> dis(-scene_size, scene_size);
	std::uniform_real_distribution<> dis_tree_scale(0.01f, 0.015f);
	std::uniform_real_distribution<> dis_rot(0, 360);

	// Create Grass
	for (std::uint32_t i = 0; i < num_grass_nodes; i++)
	{
		auto grass_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_grass_model);
		sg::helper::SetScale(m_scene_graph, grass_node, glm::vec3(0.01f, 0.01f, 0.01f));
		sg::helper::SetRotation(m_scene_graph, grass_node, glm::vec3(-90._deg, glm::degrees(dis_rot(gen)), 0));
		sg::helper::SetPosition(m_scene_graph, grass_node, glm::vec3(dis(gen), 0, dis(gen)));
	}

	// Create Trees
	for (std::uint32_t i = 0; i < num_tree_nodes; i++)
	{
		auto tree_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_tree_model);
		sg::helper::SetScale(m_scene_graph, tree_node, glm::vec3(dis_tree_scale(gen)));
		sg::helper::SetRotation(m_scene_graph, tree_node, glm::vec3(-90._deg, glm::degrees(dis_rot(gen)), 0));
		sg::helper::SetPosition(m_scene_graph, tree_node, glm::vec3(dis(gen), 0, dis(gen)));
	}
}

void ForrestScene::Update_Impl(float delta, float time)
{
}
