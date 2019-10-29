/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "fps_camera.hpp"

#include <GLFW/glfw3.h>

#include "application.hpp"
#include "scene_graph/scene_graph.hpp"

FPSCamera::FPSCamera(Settings settings)
	: m_settings(settings), m_kb_axis(0), m_rmb(false), m_last_mouse_pos(0)
{
}

void FPSCamera::Update(float delta)
{
	if (auto error = m_settings.IsValid(); error.has_value())
	{
		LOGW("FPS Camera Invalid Settings: {}", error.value());
		return;
	}

	float speed = m_settings.m_move_speed * delta;
	auto forward_right = sg::helper::GetForwardRight(m_settings.m_scene_graph, m_settings.m_camera_handle);
	sg::helper::Translate(m_settings.m_scene_graph, m_settings.m_camera_handle, (m_kb_axis.z * speed) * forward_right.first);
	sg::helper::Translate(m_settings.m_scene_graph, m_settings.m_camera_handle, (m_kb_axis.y * speed) * glm::vec3(0, 1, 0));
	sg::helper::Translate(m_settings.m_scene_graph, m_settings.m_camera_handle, (m_kb_axis.x * speed) * forward_right.second);
}

void FPSCamera::HandleKeyboardInput(int key, int action)
{
	if (auto error = m_settings.IsValid(); error.has_value())
	{
		LOGW("FPS Camera Invalid Settings: {}", error.value());
		return;
	}

	if (m_settings.IsValid())

	if (!m_rmb) return;

	float axis_mod = 0;
	if (action == GLFW_PRESS)
	{
		axis_mod = 1;
	}
	else if (action == GLFW_RELEASE)
	{
		axis_mod = -1;
	}

	if (key == GLFW_KEY_W)
	{
		m_kb_axis.z += axis_mod;
	}
	else if (key == GLFW_KEY_S)
	{
		m_kb_axis.z -= axis_mod;
	}
	else if (key == GLFW_KEY_A)
	{
		m_kb_axis.x -= axis_mod;
	}
	else if (key == GLFW_KEY_D)
	{
		m_kb_axis.x += axis_mod;
	}
	else if (key == GLFW_KEY_SPACE)
	{
		m_kb_axis.y += axis_mod;
	}
	else if (key == GLFW_KEY_LEFT_CONTROL)
	{
		m_kb_axis.y -= axis_mod;
	}

	m_kb_axis = glm::clamp(m_kb_axis, glm::vec3(-1), glm::vec3(1));
}

void FPSCamera::HandleMouseButtons(int key, int action)
{
	if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		m_rmb = true;
		m_last_mouse_pos = m_settings.m_app->GetMousePos();
		glm::vec2 center = glm::vec2(m_settings.m_app->GetWidth() / 2.f, m_settings.m_app->GetHeight() / 2.f);

		m_settings.m_app->SetMousePos(center.x, center.y);
		m_settings.m_app->SetMouseVisibility(false);
	}
	else if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		m_rmb = false;
		m_settings.m_app->SetMouseVisibility(true);
		m_settings.m_app->SetMousePos(m_last_mouse_pos.x, m_last_mouse_pos.y);

		m_kb_axis = glm::vec3(0);
	}
}

void FPSCamera::HandleMousePosition(float delta, float x, float y)
{
	if (!m_rmb) return;

	glm::vec2 center = glm::vec2(m_settings.m_app->GetWidth() / 2.f, m_settings.m_app->GetHeight() / 2.f);

	float x_movement = x - center.x;
	float y_movement = center.y - y;
	float sensitivity = m_settings.m_mouse_sensitivity * delta;

	sg::helper::Rotate(m_settings.m_scene_graph, m_settings.m_camera_handle, glm::vec3(y_movement * sensitivity, x_movement * sensitivity, 0));

	auto rotation = sg::helper::GetRotation(m_settings.m_scene_graph, m_settings.m_camera_handle);

	if (glm::degrees(rotation.x) > 89.0f)
		rotation.x = glm::radians(89.0f);
	if (glm::degrees(rotation.x) < -89.0f)
		rotation.x = -glm::radians(89.0f);

	sg::helper::SetRotation(m_settings.m_scene_graph, m_settings.m_camera_handle, rotation);

	m_settings.m_app->SetMousePos(center.x, center.y);
}

void FPSCamera::HandleControllerInput(float delta)
{
	if (auto error = m_settings.IsValid(); error.has_value())
	{
		LOGW("FPS Camera Invalid Settings: {}", error.value());
		return;
	}

	GLFWgamepadstate state;
	if (m_settings.m_app->GetGamepad(GLFW_JOYSTICK_1, &state))
	{
		float dead_zone = 0.1;
		if (!m_rmb)
		{
			m_kb_axis = { 0, 0, 0 };
		}

		// rotation
		{
			float x_axis = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
			float y_axis = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * -1;
			float sensitivity = m_settings.m_controller_sensitivity * delta;
			if (m_settings.m_flip_controller_y)
			{
				y_axis = y_axis * -1;
			}

			if (x_axis > dead_zone || y_axis > dead_zone || x_axis < -dead_zone || y_axis < -dead_zone)
			{
				sg::helper::Rotate(m_settings.m_scene_graph, m_settings.m_camera_handle, glm::vec3(y_axis * sensitivity, x_axis * sensitivity, 0));
			}
		}
		// translation
		{
			float x_axis = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			float z_axis = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * -1;
			float y_plus_axis = (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1) / 2;
			float y_minus_axis = (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1) / 2;

			if (x_axis > dead_zone || z_axis > dead_zone || x_axis < -dead_zone || z_axis < -dead_zone)
			{
				m_kb_axis.x = x_axis;
				m_kb_axis.z = z_axis;
			}

			if (y_plus_axis > dead_zone || y_plus_axis < -dead_zone)
			{
				m_kb_axis.y += y_plus_axis;
			}
			if (y_minus_axis > dead_zone || y_minus_axis < -dead_zone)
			{
				m_kb_axis.y -= y_minus_axis;
			}

			m_kb_axis = glm::clamp(m_kb_axis, glm::vec3(-1), glm::vec3(1));
		}

	}
}

bool FPSCamera::IsControlled()
{
	return m_rmb;
}

FPSCamera::Settings& FPSCamera::GetSettings()
{
	return m_settings;
}

void FPSCamera::SetApplication(Application* app)
{
	assert(app);
	m_settings.m_app = app;
}

void FPSCamera::SetSceneGraph(sg::SceneGraph* scene_graph)
{
	assert(scene_graph);
	m_settings.m_scene_graph = scene_graph;
}

void FPSCamera::SetCameraHandle(sg::NodeHandle handle)
{
	m_settings.m_camera_handle = handle;
}
