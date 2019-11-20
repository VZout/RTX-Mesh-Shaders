/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "displacement_scene.hpp"

#include <util/user_literals.hpp>
#include <vertex.hpp>
#include <future>
#include <thread>

DisplacementScene::DisplacementScene() :
	Scene("Spheres Scene", "spheres_scene.json")
{

}

void DisplacementScene::LoadResources()
{
	auto image_loader = new STBImageLoader(); // TODO: Memory leak

	auto load_texture_func = [&](std::string path) { return image_loader->LoadFromDisc(path); };

	auto albedo_f = std::async(std::launch::async, load_texture_func, "medieval_blocks/medieval_blocks_06_diff_4k.png");
	auto roughness_f = std::async(std::launch::async, load_texture_func, "medieval_blocks/medieval_blocks_06_ao_rough_metal_4k.png");
	auto displacement_f = std::async(std::launch::async, load_texture_func, "medieval_blocks/medieval_blocks_06_disp_4k.png");
	auto normal_map_f = std::async(std::launch::async, load_texture_func, "medieval_blocks/medieval_blocks_06_nor_4k.png");

	m_sphere_model = m_model_pool->LoadWithMaterials<Vertex>("sphere.fbx", m_material_pool, m_texture_pool, false);

	MaterialData mat = {};
	mat.m_base_reflectivity = 0.4f;
	mat.m_albedo_texture = *albedo_f.get();
	mat.m_roughness_texture = *roughness_f.get();
	mat.m_displacement_texture = *displacement_f.get();
	mat.m_normal_map_texture = *normal_map_f.get();
	mat.m_base_metallic = 0;

	m_sphere_material_handle = m_material_pool->Load(mat, m_texture_pool);
}

void DisplacementScene::BuildScene()
{
	// Create Camera
	m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
	sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0, 0, 5.f));
	sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(0, -90._deg, 0));

	auto sphere_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_sphere_model);
	sg::helper::SetMaterial(m_scene_graph, sphere_node, { m_sphere_material_handle });

	// Create Light
	auto light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(20, 20, 20));
	sg::helper::SetPosition(m_scene_graph, light_node, glm::vec3(0, 0, 3));
	sg::helper::SetRadius(m_scene_graph, light_node, 4);
}

void DisplacementScene::Update_Impl(float delta, float time)
{
}
