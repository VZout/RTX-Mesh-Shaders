/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <glm.hpp>
#include <optional>
#include <cstdint>
#include <limits>

#include "util/log.hpp"

class Application;
namespace sg
{
	using NodeHandle = std::uint32_t;
	class SceneGraph;
} /* sg */

class FPSCamera
{
public:
	struct Settings
	{
		Application* m_app = nullptr;
		sg::SceneGraph* m_scene_graph = nullptr;
		sg::NodeHandle m_camera_handle = std::numeric_limits<std::uint32_t>::max();

		float m_move_speed = 5;
		float m_mouse_sensitivity = 0.4;
		float m_controller_sensitivity = 2.5;
		bool m_flip_controller_y = false;

		std::optional<std::string> IsValid()
		{
			if (!m_app)
			{
				return "Settings::m_app can't be a nullptr";
			}
			else if (!m_scene_graph)
			{
				return "Settings::m_scene_graph can't be a nullptr";
			}
			else if (m_mouse_sensitivity <= 0)
			{
				return "Settings::m_mouse_sensitivity is equal or less than zero";
			}
			else if (m_move_speed <= 0)
			{
				return "Settings::m_move_speed is equal or less than zero";
			}
			else if (m_controller_sensitivity <= 0)
			{
				return "Settings::m_move_speed is equal or less than zero";
			}
			else if (m_camera_handle == std::numeric_limits<std::uint32_t>::max())
			{
				return "Settings::m_camera_handle appears to be unset";
			}

			return std::nullopt;
		}
	};

	FPSCamera(std::optional<Settings> settings = std::nullopt);

	void Update(float delta);
	void HandleKeyboardInput(int key, int action);
	void HandleMouseButtons(int key, int action);
	void HandleMousePosition(float delta, float x, float y);
	void HandleControllerInput(float delta);
	bool IsControlled();
	Settings& GetSettings();

	void SetApplication(Application* app);
	void SetSceneGraph(sg::SceneGraph* scene_graph);
	void SetCameraHandle(sg::NodeHandle handle);

private:
	Settings m_settings;

	glm::vec3 m_kb_axis;
	bool m_rmb;
	glm::vec2 m_last_mouse_pos;
};