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

	struct BasicMaterial
	{
		glm::vec3 color = glm::vec3(-1, -1, -1);
		float reflectivity = -1;
		float roughness = -1;
		float metallic = -1;
		float normal_strength = 1;
		float anisotropy = 0;
		glm::vec2 anisotropy_dir = { 1, 0 };
		float clear_coat;
		float clear_coat_roughness;
		glm::vec2 uv_scale;
		glm::vec2 padding;
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

		glm::vec3 m_direction = { 0, 0, 0 };
		float m_inner_angle = 0.698132;

		glm::vec3 m_color = { 1, 1, 1 };
		std::uint32_t m_type = (std::uint32_t)LightType::FREE;

		float m_outer_angle = 0.698132;
		glm::vec3 m_padding;
	};

	struct PrefilterInfo
	{
		float roughness;
	};

} /* CB */