/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "scene.hpp"

class ForrestScene : public Scene
{
public:
	ForrestScene();

private:
	void LoadResources(std::optional<std::reference_wrapper<util::Progress>> progress) final;
	void BuildScene(std::optional<std::reference_wrapper<util::Progress>> progress) final;
	void Update_Impl(float delta, float time) final;

	ModelHandle m_plane_model;
	ModelHandle m_object_model;
	ModelHandle m_object2_model;
	ModelHandle m_grass_model;
	ModelHandle m_tree_model;
	MaterialHandle m_plane_material_handle;
};
