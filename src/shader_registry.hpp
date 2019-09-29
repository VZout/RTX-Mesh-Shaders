/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "registry.hpp"

#include "registry.hpp"
#include "graphics/gfx_enums.hpp"
#include "graphics/shader.hpp"

struct shaders
{

	static RegistryHandle basic_ps;
	static RegistryHandle basic_vs;
	static RegistryHandle composition_cs;
	static RegistryHandle post_processing_cs;
	static RegistryHandle generate_cubemap_cs;
	static RegistryHandle generate_irradiancemap_cs;

}; /* shaders */

struct ShaderDesc
{
	std::string m_path;
	gfx::enums::ShaderType m_type;
};

class ShaderRegistry : public internal::Registry<ShaderRegistry, gfx::Shader, ShaderDesc> {};