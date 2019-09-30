/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "registry.hpp"

#include "registry.hpp"
#include "graphics/gfx_enums.hpp"
#include "graphics/pipeline_state.hpp"

struct pipelines
{

	static RegistryHandle basic;
	static RegistryHandle composition;
	static RegistryHandle post_processing;
	static RegistryHandle generate_cubemap;
	static RegistryHandle generate_irradiancemap;
	static RegistryHandle generate_environmentmap;

}; /* pipelines */

struct PipelineDesc
{
	RegistryHandle m_root_signature_handle;
	std::vector<RegistryHandle> m_shader_handles;
	std::optional<gfx::PipelineState::InputLayout> m_input_layout;

	gfx::enums::PipelineType m_type;
	std::vector<VkFormat> m_rtv_formats = {};
	VkFormat m_depth_format = VK_FORMAT_UNDEFINED;
	VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	bool m_counter_clockwise = true;
};

class PipelineRegistry : public internal::Registry<PipelineRegistry, gfx::PipelineState, PipelineDesc> {};