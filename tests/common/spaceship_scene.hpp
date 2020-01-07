/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "scene.hpp"

class SpaceshipScene : public Scene
{
public:
	SpaceshipScene();

private:
	void LoadResources(std::optional<std::reference_wrapper<util::Progress>> progress) final;
	void BuildScene(std::optional<std::reference_wrapper<util::Progress>> progress) final;
	void Update_Impl(float delta, float time) final;

	ModelHandle m_plane_model;
	ModelHandle m_spaceship_model;
	ModelHandle m_rock0_model;
};
