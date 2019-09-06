/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "render_target.hpp"

#include <stdexcept>

#include "context.hpp"

gfx::RenderTarget::RenderTarget(Context* context)
	: m_context(context), m_color_attachment(), m_subpass(),
	m_render_pass(VK_NULL_HANDLE), m_render_pass_create_info(),
	m_width(0), m_height(0)
{

}

gfx::RenderTarget::~RenderTarget()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyRenderPass(logical_device, m_render_pass, nullptr);

	for (auto buffer : m_frame_buffers)
	{
		vkDestroyFramebuffer(logical_device, buffer, nullptr);
	}
}

std::uint32_t gfx::RenderTarget::GetWidth()
{
	return m_width;
}

std::uint32_t gfx::RenderTarget::GetHeight()
{
	return m_height;
}


void gfx::RenderTarget::CreateRenderPass(VkFormat format)
{
	auto logical_device = m_context->m_logical_device;

	// Resource State description
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	m_color_attachment.format = format;
	m_color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	m_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// This attachment ref references the color attachment specified above. (referenced by index)
	// it directly references `(layout(location = 0) out vec4 out_color)` in your shader code.
	VkAttachmentReference attachment_ref = {};
	attachment_ref.attachment = 0;
	attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	m_subpass.colorAttachmentCount  = 1;
	m_subpass.pColorAttachments = &attachment_ref;

	m_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_render_pass_create_info.attachmentCount = 1;
	m_render_pass_create_info.pAttachments = &m_color_attachment;
	m_render_pass_create_info.subpassCount = 1;
	m_render_pass_create_info.pSubpasses = &m_subpass;
	m_render_pass_create_info.dependencyCount = 1;
	m_render_pass_create_info.pDependencies = &dependency;

	if (vkCreateRenderPass(logical_device, &m_render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}

}