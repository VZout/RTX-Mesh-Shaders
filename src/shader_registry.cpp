/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "shader_registry.hpp"

REGISTER(shaders::basic_vs, ShaderRegistry)({
	.m_path = "shaders/basic.vert.spv",
	.m_type = gfx::enums::ShaderType::VERTEX,
});

REGISTER(shaders::basic_ps, ShaderRegistry)({
	.m_path = "shaders/basic.frag.spv",
	.m_type = gfx::enums::ShaderType::PIXEL,
});

REGISTER(shaders::composition_cs, ShaderRegistry)({
    .m_path = "shaders/composition.comp.spv",
    .m_type = gfx::enums::ShaderType::COMPUTE,
});

REGISTER(shaders::post_processing_cs, ShaderRegistry)({
    .m_path = "shaders/post_processing.comp.spv",
    .m_type = gfx::enums::ShaderType::COMPUTE,
});