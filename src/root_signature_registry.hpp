/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "registry.hpp"

#include "registry.hpp"
#include "graphics/gfx_enums.hpp"
#include "graphics/root_signature.hpp"

struct root_signatures
{

	static RegistryHandle basic;
	static RegistryHandle composition;
	static RegistryHandle post_processing;
	static RegistryHandle generate_cubemap; // Also used by `generate_irradiance`

}; /* root_signatures */

struct RootSignatureDesc
{
	std::vector<VkDescriptorSetLayoutBinding> m_parameters;
};

class RootSignatureRegistry : public internal::Registry<RootSignatureRegistry, gfx::RootSignature, RootSignatureDesc> {};