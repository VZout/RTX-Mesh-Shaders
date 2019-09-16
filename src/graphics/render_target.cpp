/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "render_target.hpp"

#include <stdexcept>

#include "context.hpp"
#include "../util/log.hpp"

gfx::RenderTarget::RenderTarget(Context* context)
	: m_context(context), m_subpass(),
	m_render_pass(VK_NULL_HANDLE), m_render_pass_create_info(),
	m_depth_buffer_create_info(), m_depth_buffer(VK_NULL_HANDLE),
	m_depth_buffer_memory(VK_NULL_HANDLE), m_depth_buffer_view(VK_NULL_HANDLE),
	m_width(0), m_height(0)
{

}

gfx::RenderTarget::~RenderTarget()
{
	auto logical_device = m_context->m_logical_device;

	// Clean depth buffer
	if (m_depth_buffer_view != VK_NULL_HANDLE) vkDestroyImageView(logical_device, m_depth_buffer_view, nullptr);
	if (m_depth_buffer != VK_NULL_HANDLE) vkDestroyImage(logical_device, m_depth_buffer, nullptr);
	if (m_depth_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(logical_device, m_depth_buffer_memory, nullptr);

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

VkFormat gfx::RenderTarget::PickDepthFormat()
{
	// TODO: Check for support.
	return VK_FORMAT_D32_SFLOAT;
}

void gfx::RenderTarget::CreateRenderPass(VkFormat format, VkFormat depth_format)
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

	// We always have a color attachment
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment = {};
	VkAttachmentReference depth_attachment_ref = {};
	if (depth_format != VK_FORMAT_UNDEFINED)
	{
		depth_attachment.format = depth_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// This attachment ref references the color attachment specified above. (referenced by index)
	// it directly references `(layout(location = 0) out vec4 out_color)` in your shader code.
	VkAttachmentReference attachment_ref = {};
	attachment_ref.attachment = 0;
	attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_subpass = {};
	m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	m_subpass.colorAttachmentCount  = 1;
	m_subpass.pColorAttachments = &attachment_ref;
	if (depth_format != VK_FORMAT_UNDEFINED)
	{
		m_subpass.pDepthStencilAttachment = &depth_attachment_ref;
	}

	std::vector<VkAttachmentDescription> depth_and_color_attachments = {color_attachment};
	if (depth_format != VK_FORMAT_UNDEFINED)
	{
		depth_and_color_attachments.push_back(depth_attachment);
	}

	m_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_render_pass_create_info.attachmentCount = depth_and_color_attachments.size();
	m_render_pass_create_info.pAttachments = depth_and_color_attachments.data();
	m_render_pass_create_info.subpassCount = 1;
	m_render_pass_create_info.pSubpasses = &m_subpass;
	m_render_pass_create_info.dependencyCount = 1;
	m_render_pass_create_info.pDependencies = &dependency;

	if (vkCreateRenderPass(logical_device, &m_render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		LOGC("failed to create render pass!");
	}
}

void gfx::RenderTarget::CreateDepthBuffer()
{
	VkFormat depth_format = PickDepthFormat();

	auto logical_device = m_context->m_logical_device;

	m_depth_buffer_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	m_depth_buffer_create_info.imageType = VK_IMAGE_TYPE_2D;
	m_depth_buffer_create_info.extent.width = m_width;
	m_depth_buffer_create_info.extent.height = m_height;
	m_depth_buffer_create_info.extent.depth = 1;
	m_depth_buffer_create_info.mipLevels = 1;
	m_depth_buffer_create_info.arrayLayers = 1;
	m_depth_buffer_create_info.format = depth_format;
	m_depth_buffer_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	m_depth_buffer_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_depth_buffer_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	m_depth_buffer_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	m_depth_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(logical_device, &m_depth_buffer_create_info, nullptr, &m_depth_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device, m_depth_buffer, &memory_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = m_context->FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logical_device, &alloc_info, nullptr, &m_depth_buffer_memory) != VK_SUCCESS)
	{
		LOGC("failed to allocate image memory!");
	}

	vkBindImageMemory(logical_device, m_depth_buffer, m_depth_buffer_memory, 0);

	// Transition
}

void gfx::RenderTarget::CreateDepthBufferView()
{
	auto logical_device = m_context->m_logical_device;

	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = m_depth_buffer;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = m_depth_buffer_create_info.format;
	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(logical_device, &view_create_info, nullptr, &m_depth_buffer_view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}