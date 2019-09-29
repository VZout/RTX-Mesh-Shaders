/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "root_signature_registry.hpp"

REGISTER(root_signatures::basic, RootSignatureRegistry)({
	.m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
	{
		decltype(RootSignatureDesc::m_parameters) params(3);
		params[0].binding = 0; // camera
		params[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[0].descriptorCount = 1;
		params[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		params[0].pImmutableSamplers = nullptr;
		params[1].binding = 1; // root parameter 0
		params[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[1].descriptorCount = 1;
		params[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		params[1].pImmutableSamplers = nullptr;
		params[2].binding = 2; // root parameter 1
		params[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		params[2].descriptorCount = 3;
		params[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		params[2].pImmutableSamplers = nullptr;
		return params;
	}(),
});

REGISTER(root_signatures::composition, RootSignatureRegistry)({
    .m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
    {
        decltype(RootSignatureDesc::m_parameters) params(6);
	    params[0].binding = 0; // camera
	    params[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    params[0].descriptorCount = 1;
	    params[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	    params[0].pImmutableSamplers = nullptr;
	    params[1].binding = 1; // root parameter 1
	    params[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    params[1].descriptorCount = 3; // color, normal, whatever, depth
	    params[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[1].pImmutableSamplers = nullptr;
	    params[2].binding = 2; // root parameter 2
	    params[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    params[2].descriptorCount = 1;
	    params[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[2].pImmutableSamplers = nullptr;
	    params[3].binding = 3; // root parameter 3
	    params[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    params[3].descriptorCount = 1;
	    params[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[3].pImmutableSamplers = nullptr;
	    params[4].binding = 4; // skybox
	    params[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    params[4].descriptorCount = 1;
	    params[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[4].pImmutableSamplers = nullptr;
	    params[5].binding = 5; // irradiance
	    params[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    params[5].descriptorCount = 1;
	    params[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[5].pImmutableSamplers = nullptr;
        return params;
    }(),
});

REGISTER(root_signatures::post_processing, RootSignatureRegistry)({
    .m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
    {
        decltype(RootSignatureDesc::m_parameters) params(2);
	    params[0].binding = 0; // root parameter 1
	    params[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    params[0].descriptorCount = 1;
	    params[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[0].pImmutableSamplers = nullptr;
	    params[1].binding = 1; // root parameter 2
	    params[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    params[1].descriptorCount = 1;
	    params[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[1].pImmutableSamplers = nullptr;
        return params;
    }(),
});

REGISTER(root_signatures::generate_cubemap, RootSignatureRegistry)({
  .m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
  {
      decltype(RootSignatureDesc::m_parameters) params(2);
      params[0].binding = 0; // root parameter 1
      params[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      params[0].descriptorCount = 1;
      params[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      params[0].pImmutableSamplers = nullptr;
      params[1].binding = 1; // root parameter 2
      params[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      params[1].descriptorCount = 1;
      params[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      params[1].pImmutableSamplers = nullptr;
      return params;
  }(),
});