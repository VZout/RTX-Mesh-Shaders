/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "sss_scene.hpp"

#include <util/user_literals.hpp>
#include <vertex.hpp>

SubsurfaceScene::SubsurfaceScene() :
	Scene("Sub Surface Scattering Scene")
{

}

void SubsurfaceScene::LoadResources()
{
	m_bodybuilder_model = m_model_pool->LoadWithMaterials<Vertex>("bigdude_custom/PBR - Metallic Roughness SSS.gltf", m_material_pool, m_texture_pool, false);
	m_cube_model = m_model_pool->LoadWithMaterials<Vertex>("cube.fbx", m_material_pool, m_texture_pool, false);
}

void SubsurfaceScene::BuildScene()
{
	STBImageLoader* img_loader = new STBImageLoader();

	// Create Camera
	m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
	sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0, 0, 8.2f));
	sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(0, -90._deg, 0));

	// Body Builder Material
	MaterialData mat{};
	mat.m_base_color[0] = 0;
	mat.m_base_color[1] = 0;
	mat.m_base_color[2] = 0;
	mat.m_base_roughness = 0.5f;
	mat.m_base_metallic = 0;;
	mat.m_base_reflectivity = 0.5f;
	mat.m_thickness_texture = *img_loader->Load("bigdude_custom/RGB.png");
	mat.m_normal_map_texture = *img_loader->Load("bigdude_custom/blinn1SG_normal.png");
	m_bodybuilder_material = m_material_pool->Load(mat, m_texture_pool);

	// Cube Material
	MaterialData jelly_mat{};
	jelly_mat.m_base_color[0] = 0;
	jelly_mat.m_base_color[1] = 0;
	jelly_mat.m_base_color[2] = 0;
	jelly_mat.m_base_roughness = 1.f;
	jelly_mat.m_base_metallic = 0.5f;
	jelly_mat.m_base_reflectivity = 0.5f;
	jelly_mat.m_thickness_texture = *img_loader->Load("black.png");
	m_cube_material = m_material_pool->Load(jelly_mat, m_texture_pool);

	// Create Bodybuilder
	auto bodybuilder_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_bodybuilder_model);
	sg::helper::SetPosition(m_scene_graph, bodybuilder_node, {});
	sg::helper::SetScale(m_scene_graph, bodybuilder_node, glm::vec3(2));
	sg::helper::SetMaterial(m_scene_graph, bodybuilder_node, { m_bodybuilder_material, m_bodybuilder_material });

	// Create Cubes	
	{
		auto node = m_scene_graph->CreateNode<sg::MeshComponent>(m_cube_model);
		sg::helper::SetPosition(m_scene_graph, node, { 3, 0, -1 });
		sg::helper::SetScale(m_scene_graph, node, glm::vec3(4, 4, 0.1));
		sg::helper::SetMaterial(m_scene_graph, node, { m_cube_material });
	}
	{
		auto node = m_scene_graph->CreateNode<sg::MeshComponent>(m_cube_model);
		sg::helper::SetPosition(m_scene_graph, node, { -3, 0, -1 });
		sg::helper::SetScale(m_scene_graph, node, glm::vec3(4, 4, 0.1));
		sg::helper::SetMaterial(m_scene_graph, node, { m_cube_material });
	}

	// Create Light
	m_light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(15, 15, 15));
	sg::helper::SetRadius(m_scene_graph, m_light_node, 4);
}

void SubsurfaceScene::Update_Impl(float delta, float time)
{
	// animate light
	float light_x = sin(time * 2) * 4;
	sg::helper::SetPosition(m_scene_graph, m_light_node, glm::vec3(light_x, 0, -1.5));
}
