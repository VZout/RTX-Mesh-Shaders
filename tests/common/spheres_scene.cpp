/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "spheres_scene.hpp"

#include <util/user_literals.hpp>
#include <vertex.hpp>

static const unsigned num_spheres_x = 9;
static const unsigned num_spheres_y_metal_roughness = 3;
static const unsigned num_spheres_y_clear_coat = 2;
static const unsigned num_spheres_y_anisotropy = 1;
static const unsigned num_spheres_y_clear_coat_anisotropy = 1;
static const unsigned num_spheres_y =
	num_spheres_y_metal_roughness +
	num_spheres_y_clear_coat +
	num_spheres_y_anisotropy +
	num_spheres_y_clear_coat_anisotropy;

static const float sphere_scale = 0.4f;
static const float sphere_spacing = 2.1f * sphere_scale;
static const unsigned int split = 5;

SpheresScene::SpheresScene() :
	Scene("Spheres Scene", "spheres_scene.json")
{

}

void SpheresScene::LoadResources(std::optional<std::reference_wrapper<util::Progress>> progress)
{
	m_sphere_model = m_model_pool->LoadWithMaterials<Vertex>("sphere.fbx", m_material_pool, m_texture_pool, false);

	m_sphere_materials.resize(num_spheres_x * num_spheres_y);
	m_sphere_material_handles.resize(num_spheres_x * num_spheres_y);
	int i = 0;
	for (auto x = 0; x < num_spheres_x; x++)
	{
		for (auto y = 0; y < num_spheres_y_metal_roughness; y++)
		{
			MaterialData mat{};
			mat.m_base_color[0] = 1.000;
			mat.m_base_color[1] = 0.766;
			mat.m_base_color[2] = 0.336;
			mat.m_base_roughness = x / (float)(num_spheres_x - 1);
			mat.m_base_metallic = y / (float)(num_spheres_y_metal_roughness - 1);
			mat.m_base_reflectivity = 0.5f;

			m_sphere_material_handles[i] = m_material_pool->Load(mat, m_texture_pool);
			m_sphere_materials[i] = mat;
			i++;
		}

		for (auto y = 0; y < num_spheres_y_clear_coat; y++)
		{
			srand(x);
			float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

			MaterialData mat{};
			mat.m_base_color[0] = r;
			mat.m_base_color[1] = g;
			mat.m_base_color[2] = b;
			mat.m_base_roughness = 0.5;
			mat.m_base_metallic = x % 2;

			mat.m_base_clear_coat_roughness = x / (float)(num_spheres_x - 1);
			mat.m_base_clear_coat = y / (float)(num_spheres_y_clear_coat - 1);
			mat.m_base_reflectivity = 0.5f;

			m_sphere_material_handles[i] = m_material_pool->Load(mat, m_texture_pool);
			m_sphere_materials[i] = mat;
			i++;
		}

		for (auto y = 0; y < num_spheres_y_anisotropy; y++)
		{
			MaterialData mat{};
			mat.m_base_color[0] = 135 / 255.f;
			mat.m_base_color[1] = 92 / 255.f;
			mat.m_base_color[2] = 60 / 255.f;
			mat.m_base_roughness = x / (float)(num_spheres_x - 1);
			mat.m_base_anisotropy = 0.4f;
			mat.m_base_reflectivity = 0.5f;

			m_sphere_material_handles[i] = m_material_pool->Load(mat, m_texture_pool);
			m_sphere_materials[i] = mat;
			i++;
		}

		for (auto y = 0; y < num_spheres_y_clear_coat_anisotropy; y++)
		{
			MaterialData mat{};
			mat.m_base_color[0] = 39.f / 255.f;
			mat.m_base_color[1] = 4.f / 255.f;
			mat.m_base_color[2] = 61.f / 255.f;
			mat.m_base_roughness = x / (float)(num_spheres_x - 1);
			mat.m_base_metallic = 1;
			mat.m_base_anisotropy = 0.6f;
			mat.m_base_clear_coat = 0.9f;
			mat.m_base_clear_coat_roughness = 0.1f;
			mat.m_base_reflectivity = 0.7f;
			mat.m_base_anisotropy_dir = glm::vec3(1, 1, 0);

			m_sphere_material_handles[i] = m_material_pool->Load(mat, m_texture_pool);
			m_sphere_materials[i] = mat;
			i++;
		}
	}

	// Light Sphere Material
	MaterialData sphere_mat{};
	sphere_mat.m_base_color[0] = 10;
	sphere_mat.m_base_color[1] = 10;
	sphere_mat.m_base_color[2] = 10;
	sphere_mat.m_base_roughness = 1.f;
	sphere_mat.m_base_metallic = 0;;
	sphere_mat.m_base_reflectivity = 0.5f;
	m_light_sphere_material = m_material_pool->Load(sphere_mat, m_texture_pool);
}

void SpheresScene::BuildScene(std::optional<std::reference_wrapper<util::Progress>> progress)
{
	// Create Camera
	m_camera_node = m_scene_graph->CreateNode<sg::CameraComponent>();
	sg::helper::SetPosition(m_scene_graph, m_camera_node, glm::vec3(0, 0, 8.2f));
	sg::helper::SetRotation(m_scene_graph, m_camera_node, glm::vec3(0, -90._deg, 0));

	// Create Spheres
	int i = 0;
	for (auto x = 0; x < num_spheres_x; x++)
	{
		for (auto y = 0; y < num_spheres_y; y++)
		{
			float start_x = -sphere_spacing * (num_spheres_x / 2);
			float start_y = -sphere_spacing * (num_spheres_y / 2);

			auto sphere_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_sphere_model);
			sg::helper::SetPosition(m_scene_graph, sphere_node, { start_x + (sphere_spacing * x), start_y + (sphere_spacing * y), 0 });
			sg::helper::SetScale(m_scene_graph, sphere_node, glm::vec3(sphere_scale));
			sg::helper::SetMaterial(m_scene_graph, sphere_node, { m_sphere_material_handles[i] });

			i++;
		}
	}

	// Create Light
	m_light_node = m_scene_graph->CreateNode<sg::LightComponent>(cb::LightType::POINT, glm::vec3(20, 20, 20));
	sg::helper::SetPosition(m_scene_graph, m_light_node, glm::vec3(0, 0, 2));
	sg::helper::SetRadius(m_scene_graph, m_light_node, 4);

	// Create Light Sphere
	m_light_sphere_node = m_scene_graph->CreateNode<sg::MeshComponent>(m_sphere_model);
	sg::helper::SetScale(m_scene_graph, m_light_sphere_node, glm::vec3(0.1f));
	sg::helper::SetMaterial(m_scene_graph, m_light_sphere_node, { m_light_sphere_material });
}

void SpheresScene::Update_Impl(float delta, float time)
{
	// animate light
	float light_x = sin(time * 2) * 2;
	float light_y = cos(time * 2) * 2;
	//sg::helper::SetPosition(m_scene_graph, m_light_node, glm::vec3(light_x, light_y, 2));
	//sg::helper::SetPosition(m_scene_graph, m_light_sphere_node, glm::vec3(light_x, light_y, 2));
}
