/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "pipeline_registry.hpp"

#include "shader_registry.hpp"
#include "vertex.hpp"
#include "root_signature_registry.hpp"

REGISTER(pipelines::basic, PipelineRegistry)({
	.m_root_signature_handle = root_signatures::basic,
	.m_shader_handles = { shaders::basic_vs, shaders::basic_ps },
	.m_input_layout = Vertex::GetInputLayout(),

	.m_type = gfx::enums::PipelineType::GRAPHICS_PIPE,
	.m_rtv_formats = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
    .m_depth_format = VK_FORMAT_D32_SFLOAT,
	.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	.m_counter_clockwise = true
});

REGISTER(pipelines::composition, PipelineRegistry)({
     .m_root_signature_handle = root_signatures::composition,
     .m_shader_handles = { shaders::composition_cs },
     .m_input_layout = std::nullopt,

     .m_type = gfx::enums::PipelineType::COMPUTE_PIPE,
 });

REGISTER(pipelines::post_processing, PipelineRegistry)({
   .m_root_signature_handle = root_signatures::post_processing,
   .m_shader_handles = { shaders::post_processing_cs },
   .m_input_layout = std::nullopt,

   .m_type = gfx::enums::PipelineType::COMPUTE_PIPE,
});