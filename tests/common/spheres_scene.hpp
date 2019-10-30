/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "scene.hpp"

class SpheresScene : public Scene
{
public:
	SpheresScene();

private:
	void LoadResources() final;
	void BuildScene() final;
	void Update_Impl(float delta, float time) final;

	ModelHandle m_sphere_model;
	std::vector<sg::NodeHandle> m_sphere_nodes;
	std::vector<MaterialHandle> m_sphere_material_handles;
	std::vector<MaterialData> m_sphere_materials;
	sg::NodeHandle m_light_node;
};
