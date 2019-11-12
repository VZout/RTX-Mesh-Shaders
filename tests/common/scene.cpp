/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "scene.hpp"

#include <cassert>
#include <limits>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>

Scene::Scene(std::string const & name, std::optional<std::string> const & json_path) :
	m_model_pool(nullptr),
	m_texture_pool(nullptr),
	m_material_pool(nullptr),
	m_scene_graph(nullptr),
	m_camera_node(std::numeric_limits<std::uint32_t>::max()),
	m_json_path(json_path),
	m_name(name)
{

}

Scene::~Scene()
{
	delete m_scene_graph;
}

void Scene::Init(Renderer* renderer)
{
	assert(renderer);

	m_model_pool = renderer->GetModelPool();
	m_texture_pool = renderer->GetTexturePool();
	m_material_pool = renderer->GetMaterialPool();

	LoadResources();

	m_scene_graph = new sg::SceneGraph(renderer);

	BuildScene();
}

void Scene::Update(std::uint32_t frame_idx, float delta, float time)
{
	Update_Impl(delta, time);
	m_scene_graph->Update(frame_idx);
}

sg::SceneGraph* Scene::GetSceneGraph()
{
	return m_scene_graph;
}

sg::NodeHandle Scene::GetCameraNodeHandle() const
{
	return m_camera_node;
}

std::string const & Scene::GetName() const
{
	return m_name;
}

void Scene::LoadSceneFromJSON()
{
	if (!m_json_path.has_value())
	{
		LOGW("Tried to load lights from json without a path specified.");
		return;
	}

	// Read JSON file.
	std::ifstream f(m_json_path.value());
	nlohmann::json json;
	f >> json;

	// Loop over lights
	auto j_lights = json["lights"];
	auto j_meshes = json["meshes"];

	LOG("Number of lights: {}", j_lights.size());
	LOG("Number of meshes: {}", j_meshes.size());

	auto const& nodes = m_scene_graph->GetNodes();

	for (std::size_t i = 0; i < j_meshes.size(); i++)
	{
		auto j_mesh = j_meshes[i];

		auto handle = j_mesh["handle"].get<sg::NodeHandle>();
		auto pos = j_mesh["pos"].get<std::vector<float>>();
		auto rot = j_mesh["rotation"].get<std::vector<float>>();
		auto scale = j_mesh["scale"].get<std::vector<float>>();

		sg::helper::SetPosition(m_scene_graph, handle, { pos[0], pos[1], pos[2] });
		sg::helper::SetRotation(m_scene_graph, handle, { rot[0], rot[1], rot[2] });
		sg::helper::SetScale(m_scene_graph, handle, { scale[0], scale[1], scale[2] });
	}
}

void Scene::SaveSceneToJSON()
{
	if (!m_json_path.has_value())
	{
		LOGW("Tried to save lights to json without a path specified.");
		return;
	}

	nlohmann::json j_lights = nlohmann::json::array();
	nlohmann::json j_meshes = nlohmann::json::array();

	auto node_handles = m_scene_graph->GetNodeHandles();

	std::vector<nlohmann::json> v_lights();
	for (std::size_t i = 0; i < node_handles.size(); i++)
	{
		auto node = m_scene_graph->GetNode(node_handles[i]);

		// Lights
		if (node.m_light_component > -1)
		{
			auto light_component = node.m_light_component;
			auto transform_component = node.m_transform_component;
			nlohmann::json j_light = nlohmann::json::object();

			auto pos = m_scene_graph->m_positions[transform_component].m_value;
			auto scale = m_scene_graph->m_scales[transform_component].m_value;
			auto rotation = m_scene_graph->m_rotations[transform_component].m_value;

			j_light["handle"] = node_handles[i];
			j_light["type"] = (int)m_scene_graph->m_light_types[light_component].m_value;
			j_light["pos"] = { pos.x, pos.y, pos.z };
			j_light["scale"] = { scale.x, scale.y, scale.z };
			j_light["rotation"] = { rotation.x, rotation.y, rotation.z };
			j_light["radius"] = m_scene_graph->m_radius[light_component].m_value;
			j_light["angles"] = m_scene_graph->m_light_angles[light_component].m_value;

			j_lights.push_back(j_light);
		}
		// Meshes
		else if (node.m_mesh_component > -1)
		{
			auto transform_component = node.m_transform_component;
			nlohmann::json j_mesh = nlohmann::json::object();

			auto pos = m_scene_graph->m_positions[transform_component].m_value;
			auto scale = m_scene_graph->m_scales[transform_component].m_value;
			auto rotation = m_scene_graph->m_rotations[transform_component].m_value;

			j_mesh["handle"] = node_handles[i];
			j_mesh["pos"] = { pos.x, pos.y, pos.z };
			j_mesh["scale"] = { scale.x, scale.y, scale.z };
			j_mesh["rotation"] = { rotation.x, rotation.y, rotation.z };

			j_meshes.push_back(j_mesh);
		}
	}

	// write JSON to  file
	nlohmann::json json;
	json["lights"] = j_lights;
	json["meshes"] = j_meshes;

	std::ofstream o(m_json_path.value());
	o << std::setw(2) << json << std::endl;
}
