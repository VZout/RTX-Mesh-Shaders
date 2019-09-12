/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "pipeline_state.hpp"

#include "../util/log.hpp"
#include "context.hpp"
#include "viewport.hpp"
#include "root_signature.hpp"
#include "gfx_settings.hpp"
#include "shader.hpp"
#include "render_target.hpp"

gfx::PipelineState::PipelineState(Context* context)
	: m_context(context),
	m_vertex_input_info(),
	m_ia_info(),
	m_viewport_info(),
	m_raster_info(),
	m_ms_info(),
	m_color_blend_attachment_info(),
	m_color_blend_info(),
	m_layout(VK_NULL_HANDLE),
	m_root_signature(nullptr),
	m_render_target(nullptr),
	m_create_info(),
	m_pipeline(VK_NULL_HANDLE),
	m_input_layout()
{
	// Input Assembler
	m_ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	m_ia_info.primitiveRestartEnable = VK_FALSE;

	// Rasterizer
	m_raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_raster_info.depthClampEnable = VK_FALSE;
	m_raster_info.rasterizerDiscardEnable = VK_FALSE;
	m_raster_info.polygonMode = VK_POLYGON_MODE_FILL;
	m_raster_info.lineWidth = 1.0f;
	m_raster_info.cullMode = gfx::settings::cull_mode;
	m_raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_raster_info.depthBiasEnable = VK_FALSE;
	m_raster_info.depthBiasConstantFactor = 0.0f;
	m_raster_info.depthBiasClamp = 0.0f;
	m_raster_info.depthBiasSlopeFactor = 0.0f;

	// Multi-Sampling
	m_ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_ms_info.sampleShadingEnable = VK_FALSE;
	m_ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_ms_info.minSampleShading = 1.0f;
	m_ms_info.pSampleMask = nullptr;
	m_ms_info.alphaToCoverageEnable = VK_FALSE;
	m_ms_info.alphaToOneEnable = VK_FALSE;

	// Color Blend Attachment
	m_color_blend_attachment_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_color_blend_attachment_info.blendEnable = VK_TRUE;
	m_color_blend_attachment_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_color_blend_attachment_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_color_blend_attachment_info.colorBlendOp = VK_BLEND_OP_ADD;
	m_color_blend_attachment_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_color_blend_attachment_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_color_blend_attachment_info.alphaBlendOp = VK_BLEND_OP_ADD;

	// Color Blend
	m_color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_color_blend_info.logicOpEnable = VK_FALSE;
	m_color_blend_info.logicOp = VK_LOGIC_OP_COPY;
	m_color_blend_info.attachmentCount = 1;
	m_color_blend_info.pAttachments = &m_color_blend_attachment_info;
	m_color_blend_info.blendConstants[0] = 0.0f;
	m_color_blend_info.blendConstants[1] = 0.0f;
	m_color_blend_info.blendConstants[2] = 0.0f;
	m_color_blend_info.blendConstants[3] = 0.0f;
}

gfx::PipelineState::~PipelineState()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyPipeline(logical_device, m_pipeline, nullptr);
}

void gfx::PipelineState::SetViewport(Viewport* viewport)
{
	m_viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewport_info.viewportCount = 1;
	m_viewport_info.pViewports = &viewport->m_viewport;
	m_viewport_info.scissorCount = 1;
	m_viewport_info.pScissors = &viewport->m_scissor;
}

void gfx::PipelineState::SetRootSignature(RootSignature* root_signature)
{
	m_root_signature = root_signature;
}

void gfx::PipelineState::AddShader(gfx::Shader* shader)
{
	m_shader_info.push_back(shader->m_shader_stage_create_info);
}

void gfx::PipelineState::SetRenderTarget(gfx::RenderTarget* target)
{
	m_render_target = target;
}

void gfx::PipelineState::SetInputLayout(InputLayout const & input_layout)
{
	m_input_layout = input_layout;
}

void gfx::PipelineState::Compile()
{
	auto logical_device = m_context->m_logical_device;

	// Vertex Input
	m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertex_input_info.vertexBindingDescriptionCount = m_input_layout.first.size();
	m_vertex_input_info.pVertexBindingDescriptions = m_input_layout.first.data();
	m_vertex_input_info.vertexAttributeDescriptionCount = m_input_layout.second.size();
	m_vertex_input_info.pVertexAttributeDescriptions = m_input_layout.second.data();

	m_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	m_create_info.stageCount = m_shader_info.size();
	m_create_info.pStages = m_shader_info.data();
	m_create_info.pVertexInputState = &m_vertex_input_info;
	m_create_info.pInputAssemblyState = &m_ia_info;
	m_create_info.pViewportState = &m_viewport_info;
	m_create_info.pRasterizationState = &m_raster_info;
	m_create_info.pMultisampleState = &m_ms_info;
	m_create_info.pDepthStencilState = nullptr;
	m_create_info.pColorBlendState = &m_color_blend_info;
	m_create_info.pDynamicState = nullptr;
	m_create_info.layout = m_root_signature->m_pipeline_layout;
	m_create_info.renderPass = m_render_target->m_render_pass;
	m_create_info.subpass = 0;
	m_create_info.basePipelineHandle = VK_NULL_HANDLE;
	m_create_info.basePipelineIndex = -1;
	m_create_info.flags = 0;

	if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &m_create_info, nullptr, &m_pipeline) != VK_SUCCESS)
	{
		LOGC("failed to create graphics pipeline!");
	}
}

void gfx::PipelineState::Recompile()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyPipeline(logical_device, m_pipeline, nullptr);

	Compile();
}
