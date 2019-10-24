/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "util/defines.hpp"
#include "graphics/pipeline_state.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <vec2.hpp>
#include <vec3.hpp>

struct Vertex2D
{
	glm::vec2 m_pos;
	glm::vec3 m_color;

	static gfx::PipelineState::InputLayout GetInputLayout()
	{
		std::vector<VkVertexInputBindingDescription> binding_descs(1);
		binding_descs[0].binding = 0;
		binding_descs[0].stride = sizeof(Vertex2D);
		binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attribute_descs(2);
		// position attribute
		attribute_descs[0].binding = 0;
		attribute_descs[0].location = 0;
		attribute_descs[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descs[0].offset = offsetof(Vertex2D, m_pos);
		// color attribute
		attribute_descs[1].binding = 0;
		attribute_descs[1].location = 1;
		attribute_descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[1].offset = offsetof(Vertex2D, m_color);

		return { binding_descs, attribute_descs };
	}
};

struct Vertex
{
	glm::vec3 m_pos;
	glm::vec2 m_uv;
	glm::vec3 m_normal;
	glm::vec3 m_tangent;
	glm::vec3 m_bitangent;
	//glm::vec2 padding;

	static gfx::PipelineState::InputLayout GetInputLayout()
	{
		std::vector<VkVertexInputBindingDescription> binding_descs(1);
		binding_descs[0].binding = 0;
		binding_descs[0].stride = sizeof(Vertex);
		binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attribute_descs(5);
		// position attribute
		attribute_descs[0].binding = 0;
		attribute_descs[0].location = 0;
		attribute_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[0].offset = offsetof(Vertex, m_pos);
		// color attribute
		attribute_descs[1].binding = 0;
		attribute_descs[1].location = 1;
		attribute_descs[1].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descs[1].offset = offsetof(Vertex, m_uv);
		// color attribute
		attribute_descs[2].binding = 0;
		attribute_descs[2].location = 2;
		attribute_descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[2].offset = offsetof(Vertex, m_normal);
		// tangent
		attribute_descs[3].binding = 0;
		attribute_descs[3].location = 3;
		attribute_descs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[3].offset = offsetof(Vertex, m_tangent);
		// bitangent
		attribute_descs[4].binding = 0;
		attribute_descs[4].location = 4;
		attribute_descs[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[4].offset = offsetof(Vertex, m_bitangent);

		return { binding_descs, attribute_descs };
	}
};

IS_PROPER_VERTEX_CLASS(Vertex2D)
IS_PROPER_VERTEX_CLASS(Vertex)