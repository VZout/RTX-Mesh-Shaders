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
		float reflectivity = 0.5F;
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

	struct CameraInverse
	{
		//alignas(16) glm::mat4 m_view;
		//alignas(16) glm::mat4 m_proj;
		glm::vec4 cameraPositionAspect;
		glm::vec4 cameraUpVectorTanHalfFOV;
		glm::vec4 cameraRightVectorLensR;
		glm::vec4 cameraForwardVectorLensF;
	};

	enum class LightType : int
	{
		POINT, DIRECTIONAL, SPOT
	};

	struct Light
	{
		glm::vec3 m_pos = { 0, 0, 0 };
		float m_radius = 0.0f;

		glm::vec3 m_direction = { 0, 0, 0 };
		float m_inner_angle = 0.698132;

		glm::vec3 m_color = { 1, 1, 1 };

		//   m_x        | Bits | Content
		//  ------------|:----:|----------------------------------------------------------------
		//  light type  | 2    | value in the range of 0 - 3 determining the type of the light.
		//  num lights  | 30   | the number of lights (only stored in the first light)
		std::uint32_t m_type = 0;

		float m_outer_angle = 0.698132;
		float m_physical_size = 0;
		glm::vec2 m_padding;
	};

	struct PrefilterInfo
	{
		float roughness;
	};

} /* CB */