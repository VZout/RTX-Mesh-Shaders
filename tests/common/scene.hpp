/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <scene_graph/scene_graph.hpp>
#include <renderer.hpp>
#include <cstdint>
#include <optional>
#include <util/progress.hpp>
#include <utility>

class Scene
{
public:
	Scene(std::string const & name, std::optional<std::string> const & json_path = std::nullopt);
	virtual ~Scene();

	virtual void Init(Renderer* renderer, std::optional<std::reference_wrapper<util::Progress>> progress = std::nullopt);
	virtual void Update(std::uint32_t frame_idx, float delta, float time);
	sg::SceneGraph* GetSceneGraph();
	sg::NodeHandle GetCameraNodeHandle() const;
	std::string const & GetName() const;


	void LoadSceneFromJSON();
	void SaveSceneToJSON();

protected:
	virtual void LoadResources(std::optional<std::reference_wrapper<util::Progress>> progress) = 0;
	virtual void BuildScene(std::optional<std::reference_wrapper<util::Progress>> progress) = 0;
	virtual void Update_Impl(float delta, float time) = 0;

	ModelPool* m_model_pool;
	TexturePool* m_texture_pool;
	MaterialPool* m_material_pool;

	sg::SceneGraph* m_scene_graph;

	sg::NodeHandle m_camera_node;

	const std::optional<std::string> m_json_path;

	const std::string m_name;
};
