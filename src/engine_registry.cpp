/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "engine_registry.hpp"

#include "vertex.hpp"
#include "graphics/gfx_settings.hpp"

/* ============================================================== */
/* ===                    Shader Registry                     === */
/* ============================================================== */

REGISTER(shaders::basic_vs, ShaderRegistry)({
	.m_path = "shaders/basic.vert.spv",
	.m_type = gfx::enums::ShaderType::VERTEX,
});

REGISTER(shaders::basic_ps, ShaderRegistry)({
	.m_path = "shaders/basic.frag.spv",
	.m_type = gfx::enums::ShaderType::PIXEL,
});

REGISTER(shaders::basic_mesh, ShaderRegistry)({
	.m_path = "shaders/basic_mesh.comp.spv",
	.m_type = gfx::enums::ShaderType::MESH,
});

REGISTER(shaders::instancing_task, ShaderRegistry)({
   .m_path = "shaders/instancing_task.comp.spv",
   .m_type = gfx::enums::ShaderType::TASK,
});

REGISTER(shaders::composition_cs, ShaderRegistry)({
    .m_path = "shaders/composition.comp.spv",
    .m_type = gfx::enums::ShaderType::COMPUTE,
});

REGISTER(shaders::post_processing_cs, ShaderRegistry)({
    .m_path = "shaders/post_processing.comp.spv",
    .m_type = gfx::enums::ShaderType::COMPUTE,
});

REGISTER(shaders::generate_cubemap_cs, ShaderRegistry)({
    .m_path = "shaders/generate_cubemap.comp.spv",
    .m_type = gfx::enums::ShaderType::COMPUTE,
});

REGISTER(shaders::generate_irradiancemap_cs, ShaderRegistry)({
   .m_path = "shaders/generate_irradiancemap.comp.spv",
   .m_type = gfx::enums::ShaderType::COMPUTE,
});

REGISTER(shaders::generate_environmentmap_cs, ShaderRegistry)({
   .m_path = "shaders/generate_environmentmap.comp.spv",
   .m_type = gfx::enums::ShaderType::COMPUTE,
});

REGISTER(shaders::generate_brdf_lut_cs, ShaderRegistry)({
   .m_path = "shaders/generate_brdf_lut.comp.spv",
   .m_type = gfx::enums::ShaderType::COMPUTE,
});

/* ============================================================== */
/* ===                  RootSignature Registry                === */
/* ============================================================== */

REGISTER(root_signatures::basic, RootSignatureRegistry)({
	.m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
	{
		decltype(RootSignatureDesc::m_parameters) params(4);
		params[0].binding = 0; // camera
		params[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[0].descriptorCount = 1;
		params[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
		params[0].pImmutableSamplers = nullptr;
		params[1].binding = 1; // root parameter 0
		params[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[1].descriptorCount = 1;
		params[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
		params[1].pImmutableSamplers = nullptr;
		params[2].binding = 2; // root parameter 1
		params[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		params[2].descriptorCount = 5; // textures
		params[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
		params[2].pImmutableSamplers = nullptr;
		params[3].binding = 3; // root parameter 0
		params[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[3].descriptorCount = 1;
		params[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		params[3].pImmutableSamplers = nullptr;
		return params;
	}(),
});

REGISTER(root_signatures::basic_mesh, RootSignatureRegistry)({
	.m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
	{
		decltype(RootSignatureDesc::m_parameters) params(7);
		params[0].binding = 0; // camera
		params[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[0].descriptorCount = 1;
		params[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
		params[0].pImmutableSamplers = nullptr;
		params[1].binding = 1; // root parameter 0
		params[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[1].descriptorCount = 1;
		params[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
		params[1].pImmutableSamplers = nullptr;
		params[2].binding = 2; // root parameter 1
		params[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		params[2].descriptorCount = 5; // textures
		params[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
		params[2].pImmutableSamplers = nullptr;
		params[3].binding = 3; // root parameter 2
		params[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		params[3].descriptorCount = 1;
		params[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		params[3].pImmutableSamplers = nullptr;
		params[4].binding = 4; // root parameter 3
		params[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		params[4].descriptorCount = 1;
		params[4].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;
		params[4].pImmutableSamplers = nullptr;
		params[5].binding = 5; // root parameter 4
		params[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		params[5].descriptorCount = 1;
		params[5].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;
		params[5].pImmutableSamplers = nullptr;
		params[6].binding = 6; // root parameter 5
		params[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		params[6].descriptorCount = 1;
		params[6].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;
		params[6].pImmutableSamplers = nullptr;
		return params;
	}(),
	.m_push_constants = []() -> decltype(RootSignatureDesc::m_push_constants)
	{
		decltype(RootSignatureDesc::m_push_constants) constants(1);
		constants[0].offset = 0;
		constants[0].size = sizeof(unsigned int) * 2; // meshlet offset (instance) and meshlet count
		constants[0].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV;
		return constants;
	}()
});


REGISTER(root_signatures::composition, RootSignatureRegistry)({
    .m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
    {
        decltype(RootSignatureDesc::m_parameters) params(8);
	    params[0].binding = 0; // camera
	    params[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    params[0].descriptorCount = 1;
	    params[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_NV;
	    params[0].pImmutableSamplers = nullptr;
	    params[1].binding = 1; // root parameter 1
	    params[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    params[1].descriptorCount = 5; // color, normal, pos, material, anisotropy
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
	    params[5].binding = 5; // irradiance map
	    params[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    params[5].descriptorCount = 1;
	    params[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[5].pImmutableSamplers = nullptr;
	    params[6].binding = 6; // envionment map
	    params[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    params[6].descriptorCount = 1;
	    params[6].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[6].pImmutableSamplers = nullptr;
	    params[7].binding = 7; // envionment map
	    params[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    params[7].descriptorCount = 1;
	    params[7].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	    params[7].pImmutableSamplers = nullptr;
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

REGISTER(root_signatures::generate_environmentmap, RootSignatureRegistry)({
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
  .m_push_constants = []() -> decltype(RootSignatureDesc::m_push_constants)
    {
	    decltype(RootSignatureDesc::m_push_constants) constants(1);
	    constants[0].offset = 0;
	    constants[0].size = sizeof(float) * 2; // roughness and face
	    constants[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  	    return constants;
	}()
});

REGISTER(root_signatures::generate_brdf_lut, RootSignatureRegistry)({
  .m_parameters = []() -> decltype(RootSignatureDesc::m_parameters)
  {
      decltype(RootSignatureDesc::m_parameters) params(1);
      params[0].binding = 0; // root parameter 1
      params[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      params[0].descriptorCount = 1;
      params[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      params[0].pImmutableSamplers = nullptr;
      return params;
  }(),
});

/* ============================================================== */
/* ===                    Pipeline Registry                   === */
/* ============================================================== */

REGISTER(pipelines::basic, PipelineRegistry)({
	.m_root_signature_handle = root_signatures::basic,
	.m_shader_handles = { shaders::basic_vs, shaders::basic_ps },
	.m_input_layout = Vertex::GetInputLayout(),

	.m_type = gfx::enums::PipelineType::GRAPHICS_PIPE,
	.m_rtv_formats = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
    .m_depth_format = VK_FORMAT_D32_SFLOAT,
	.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	.m_counter_clockwise = true
});

REGISTER(pipelines::basic_mesh, PipelineRegistry)({
	.m_root_signature_handle = root_signatures::basic_mesh,
	.m_shader_handles = { shaders::instancing_task, shaders::basic_mesh, shaders::basic_ps },
	.m_input_layout = std::nullopt, // mesh shading

	.m_type = gfx::enums::PipelineType::GRAPHICS_PIPE,
	.m_rtv_formats = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
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

REGISTER(pipelines::generate_cubemap, PipelineRegistry)({
   .m_root_signature_handle = root_signatures::generate_cubemap,
   .m_shader_handles = { shaders::generate_cubemap_cs },
   .m_input_layout = std::nullopt,

   .m_type = gfx::enums::PipelineType::COMPUTE_PIPE,
});

REGISTER(pipelines::generate_irradiancemap, PipelineRegistry)({
    .m_root_signature_handle = root_signatures::generate_cubemap,
    .m_shader_handles = { shaders::generate_irradiancemap_cs },
    .m_input_layout = std::nullopt,

    .m_type = gfx::enums::PipelineType::COMPUTE_PIPE,
});

REGISTER(pipelines::generate_environmentmap, PipelineRegistry)({
    .m_root_signature_handle = root_signatures::generate_environmentmap,
    .m_shader_handles = { shaders::generate_environmentmap_cs },
    .m_input_layout = std::nullopt,

    .m_type = gfx::enums::PipelineType::COMPUTE_PIPE,
});

REGISTER(pipelines::generate_brdf_lut, PipelineRegistry)({
    .m_root_signature_handle = root_signatures::generate_brdf_lut,
    .m_shader_handles = { shaders::generate_brdf_lut_cs },
    .m_input_layout = std::nullopt,

    .m_type = gfx::enums::PipelineType::COMPUTE_PIPE,
});