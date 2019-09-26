/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <mat4x4.hpp>

namespace cb
{

	struct Basic
	{
		alignas(16) glm::mat4 m_model;
	};

	struct Camera
	{
		alignas(16) glm::mat4 m_view;
		alignas(16) glm::mat4 m_proj;
	};

	enum class LightType : int
	{
		POINT, DIRECTIONAL, SPOT, FREE
	};

	struct Light
	{
		glm::vec3 m_pos = { 0, 0, 0 };
		float m_radius = 0.0f;

		glm::vec3 m_color = { 1, 1, 1 };
		std::uint32_t m_type = (std::uint32_t)LightType::FREE;
	};

} /* CB */