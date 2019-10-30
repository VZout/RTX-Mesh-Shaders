/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "scene.hpp"

#include <cassert>
#include <limits>

Scene::Scene(std::string const & name) :
	m_model_pool(nullptr),
	m_texture_pool(nullptr),
	m_material_pool(nullptr),
	m_scene_graph(nullptr),
	m_camera_node(std::numeric_limits<std::uint32_t>::max()),
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
