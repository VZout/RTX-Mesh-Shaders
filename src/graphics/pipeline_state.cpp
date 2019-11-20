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

gfx::PipelineState::PipelineState(Context* context, Desc desc)
	: m_context(context), m_desc(desc),
	m_vertex_input_info(),
	m_ia_info(),
	m_viewport_info(),
	m_raster_info(),
	m_ms_info(),
	m_depth_stencil_info(),
	m_color_blend_attachment_info(),
	m_color_blend_info(),
	m_root_signature(nullptr),
	m_pipeline(VK_NULL_HANDLE),
    m_render_pass(VK_NULL_HANDLE),
	m_input_layout()
{
	// TODO: Remove this from the constructor. This is not required for compute.
	// Input Assembler
	m_ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_ia_info.topology = m_desc.m_topology;
	m_ia_info.primitiveRestartEnable = VK_FALSE;

	// Rasterizer
	m_raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_raster_info.depthClampEnable = VK_FALSE;
	m_raster_info.rasterizerDiscardEnable = VK_FALSE;
	m_raster_info.polygonMode = VK_POLYGON_MODE_FILL;
	m_raster_info.lineWidth = 2.0f;
	m_raster_info.cullMode = gfx::settings::cull_mode;
	m_raster_info.frontFace = m_desc.m_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
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

	// Depth Stencil
	m_depth_stencil_info = {};
	m_depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depth_stencil_info.depthTestEnable = VK_TRUE;
	m_depth_stencil_info.depthWriteEnable = VK_TRUE;
	m_depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
	m_depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	m_depth_stencil_info.minDepthBounds = 0.0f; // Optional
	m_depth_stencil_info.maxDepthBounds = 1.0f; // Optional
	m_depth_stencil_info.stencilTestEnable = VK_FALSE;
	m_depth_stencil_info.front = {};
	m_depth_stencil_info.back = {};

	// Color Blend Attachment
	VkPipelineColorBlendAttachmentState color_blend_attachment_info = {};
	color_blend_attachment_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_info.blendEnable = VK_FALSE;
	color_blend_attachment_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_info.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_info.alphaBlendOp = VK_BLEND_OP_ADD;
	m_color_blend_attachment_info.resize(desc.m_rtv_formats.size(), color_blend_attachment_info);

	// Color Blend
	m_color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_color_blend_info.logicOpEnable = VK_FALSE;
	m_color_blend_info.logicOp = VK_LOGIC_OP_COPY;
	m_color_blend_info.attachmentCount = m_color_blend_attachment_info.size();
	m_color_blend_info.pAttachments = m_color_blend_attachment_info.data();
	m_color_blend_info.blendConstants[0] = 0.0f;
	m_color_blend_info.blendConstants[1] = 0.0f;
	m_color_blend_info.blendConstants[2] = 0.0f;
	m_color_blend_info.blendConstants[3] = 0.0f;
}

gfx::PipelineState::~PipelineState()
{
	Cleanup();
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

void gfx::PipelineState::SetInputLayout(InputLayout const & input_layout)
{
	m_input_layout = input_layout;
}

void gfx::PipelineState::Compile()
{
	auto logical_device = m_context->m_logical_device;

	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED || !m_desc.m_rtv_formats.empty())
	{
		CreateRenderPass();
	}

	// Vertex Input
	m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (m_input_layout.has_value())
	{
		m_vertex_input_info.vertexBindingDescriptionCount = m_input_layout->first.size();
		m_vertex_input_info.pVertexBindingDescriptions = m_input_layout->first.data();
		m_vertex_input_info.vertexAttributeDescriptionCount = m_input_layout->second.size();
		m_vertex_input_info.pVertexAttributeDescriptions = m_input_layout->second.data();
	}
	else
	{
		m_vertex_input_info.vertexBindingDescriptionCount = 0;
		m_vertex_input_info.vertexAttributeDescriptionCount = 0;
	}

	if (m_desc.m_type == enums::PipelineType::GRAPHICS_PIPE)
	{
		VkGraphicsPipelineCreateInfo m_create_info = {};
		m_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		m_create_info.stageCount = m_shader_info.size();
		m_create_info.pStages = m_shader_info.data();
		m_create_info.pVertexInputState = &m_vertex_input_info;
		m_create_info.pInputAssemblyState = &m_ia_info;
		m_create_info.pViewportState = &m_viewport_info;
		m_create_info.pRasterizationState = &m_raster_info;
		m_create_info.pMultisampleState = &m_ms_info;
		m_create_info.pDepthStencilState =
				m_desc.m_depth_format != VK_FORMAT_UNDEFINED ? &m_depth_stencil_info : nullptr;
		m_create_info.pColorBlendState = &m_color_blend_info;
		m_create_info.pDynamicState = nullptr;
		m_create_info.layout = m_root_signature->m_pipeline_layout;
		m_create_info.renderPass = m_render_pass;
		m_create_info.subpass = 0;
		m_create_info.basePipelineHandle = VK_NULL_HANDLE;
		m_create_info.basePipelineIndex = -1;
		m_create_info.flags = 0;

		if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &m_create_info, nullptr, &m_pipeline)
			!= VK_SUCCESS)
		{
			LOGC("failed to create graphics pipeline!");
		}
	}
	else if (m_desc.m_type == enums::PipelineType::COMPUTE_PIPE)
	{
		VkComputePipelineCreateInfo m_create_info = {};
		m_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		m_create_info.layout = m_root_signature->m_pipeline_layout;
		m_create_info.basePipelineHandle = VK_NULL_HANDLE;
		m_create_info.basePipelineIndex = -1;
		m_create_info.stage = m_shader_info[0];
		m_create_info.flags = 0;

		if (vkCreateComputePipelines(logical_device, VK_NULL_HANDLE, 1, &m_create_info, nullptr, &m_pipeline)
			!= VK_SUCCESS)
		{
			LOGC("failed to create compute pipeline!");
		}
	}
	else
	{
		LOGW("Unsupported pipeline type");
	}
}

void gfx::PipelineState::Recompile()
{
	Cleanup();
	Compile();
}

// This is inneficient because we create 2 render passess for everything. But this brings us closer to DX12 behaviour.
void gfx::PipelineState::CreateRenderPass()
{
	auto logical_device = m_context->m_logical_device;

	// Resource State descriptions
	std::vector<VkSubpassDependency> dependencies;

	// We always have a color attachment
	std::vector<VkAttachmentDescription> color_attachments = {};
	color_attachments.reserve(m_desc.m_rtv_formats.size());
	for (auto format : m_desc.m_rtv_formats)
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		color_attachments.push_back(color_attachment);

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies.push_back(dependency);
	}

	// This attachment ref references the color attachment specified above. (referenced by index)
	// it directly references `(layout(location = 0) out vec4 out_color)` in your shader code.
	std::vector<VkAttachmentReference> color_attachment_refs(m_desc.m_rtv_formats.size());
	for (std::size_t i = 0; i < m_desc.m_rtv_formats.size(); i++)
	{
		color_attachment_refs[i].attachment = i;
		color_attachment_refs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentDescription depth_attachment = {};
	VkAttachmentReference depth_attachment_ref = {};
	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		depth_attachment.format = m_desc.m_depth_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depth_attachment_ref.attachment = color_attachment_refs.size();
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = color_attachment_refs.size();
	subpass.pColorAttachments = color_attachment_refs.data();
	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		subpass.pDepthStencilAttachment = &depth_attachment_ref;
	}

	std::vector<VkAttachmentDescription> depth_and_color_attachments = color_attachments;
	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		depth_and_color_attachments.push_back(depth_attachment);
	}

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = depth_and_color_attachments.size();
	render_pass_create_info.pAttachments = depth_and_color_attachments.data();
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;
	render_pass_create_info.dependencyCount = dependencies.size();
	render_pass_create_info.pDependencies = dependencies.data();

	if (vkCreateRenderPass(logical_device, &render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		LOGC("failed to create render pass!");
	}
}

void gfx::PipelineState::Cleanup()
{
	auto logical_device = m_context->m_logical_device;

	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED || !m_desc.m_rtv_formats.empty())
	{
		vkDestroyRenderPass(logical_device, m_render_pass, nullptr);
	}
	vkDestroyPipeline(logical_device, m_pipeline, nullptr);
}