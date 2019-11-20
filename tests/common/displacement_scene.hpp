/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "scene.hpp"

class DisplacementScene : public Scene
{
public:
	DisplacementScene();

private:
	void LoadResources() final;
	void BuildScene() final;
	void Update_Impl(float delta, float time) final;

	ModelHandle m_sphere_model;
	MaterialHandle m_sphere_material_handle;
};
