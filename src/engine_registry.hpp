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
#include "graphics/root_signature.hpp"
#include "graphics/pipeline_state.hpp"

struct shaders
{

	static RegistryHandle basic_ps;
	static RegistryHandle basic_vs;
	static RegistryHandle composition_cs;
	static RegistryHandle post_processing_cs;
	static RegistryHandle generate_cubemap_cs;
	static RegistryHandle generate_irradiancemap_cs;
	static RegistryHandle generate_environmentmap_cs;
	static RegistryHandle generate_brdf_lut_cs;

}; /* shaders */

struct root_signatures
{

	static RegistryHandle basic;
	static RegistryHandle composition;
	static RegistryHandle post_processing;
	static RegistryHandle generate_cubemap; // Also used by `generate_irradiance`.
	static RegistryHandle generate_environmentmap;
	static RegistryHandle generate_brdf_lut;

}; /* root_signatures */

struct pipelines
{

	static RegistryHandle basic;
	static RegistryHandle composition;
	static RegistryHandle post_processing;
	static RegistryHandle generate_cubemap;
	static RegistryHandle generate_irradiancemap;
	static RegistryHandle generate_environmentmap;
	static RegistryHandle generate_brdf_lut;

}; /* pipelines */

struct ShaderDesc
{
	std::string m_path;
	gfx::enums::ShaderType m_type;
};

struct RootSignatureDesc
{
	std::vector<VkDescriptorSetLayoutBinding> m_parameters;
	std::vector<VkPushConstantRange> m_push_constants = {};
};

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

class ShaderRegistry : public internal::Registry<ShaderRegistry, gfx::Shader, ShaderDesc> {};
class PipelineRegistry : public internal::Registry<PipelineRegistry, gfx::PipelineState, PipelineDesc> {};
class RootSignatureRegistry : public internal::Registry<RootSignatureRegistry, gfx::RootSignature, RootSignatureDesc> {};