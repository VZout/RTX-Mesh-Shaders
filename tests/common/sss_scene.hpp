/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "scene.hpp"

class SubsurfaceScene : public Scene
{
public:
	SubsurfaceScene();

private:
	void LoadResources() final;
	void BuildScene() final;
	void Update_Impl(float delta, float time) final;

	ModelHandle m_bodybuilder_model;
	ModelHandle m_cube_model;
	ModelHandle m_sphere_model;
	sg::NodeHandle m_light_sphere_node;
	sg::NodeHandle m_light_node;
	MaterialHandle m_bodybuilder_material;
	MaterialHandle m_cube_material;
	MaterialHandle m_light_sphere_material;
};
