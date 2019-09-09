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
	glm::vec2 pos;
	glm::vec3 color;

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
		attribute_descs[0].offset = offsetof(Vertex2D, pos);
		// color attribute
		attribute_descs[1].binding = 0;
		attribute_descs[1].location = 1;
		attribute_descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descs[1].offset = offsetof(Vertex2D, color);

		return { binding_descs, attribute_descs };
	}
};

IS_PROPER_VERTEX_CLASS(Vertex2D)