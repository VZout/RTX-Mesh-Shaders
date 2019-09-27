/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "render_target.hpp"

#include <stdexcept>

#include "context.hpp"
#include "gfx_defines.hpp"
#include "../util/log.hpp"

gfx::RenderTarget::RenderTarget(Context* context)
	: m_context(context), m_subpass(),
	m_render_pass(VK_NULL_HANDLE), m_render_pass_create_info(),
	m_depth_buffer_create_info(), m_depth_buffer(VK_NULL_HANDLE),
	m_depth_buffer_memory(VK_NULL_HANDLE), m_depth_buffer_view(VK_NULL_HANDLE),
	m_desc()
{

}

gfx::RenderTarget::RenderTarget(Context* context, Desc desc)
		: m_context(context), m_subpass(),
		  m_render_pass(VK_NULL_HANDLE), m_render_pass_create_info(),
		  m_depth_buffer_create_info(), m_depth_buffer(VK_NULL_HANDLE),
		  m_depth_buffer_memory(VK_NULL_HANDLE), m_depth_buffer_view(VK_NULL_HANDLE),
		  m_desc(desc)
{
	CreateImages();
	CreateImageViews();

	if (desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		CreateDepthBuffer();
		CreateDepthBufferView();
	}

	if (!desc.m_allow_uav)
	{
		CreateRenderPass();

		CreateFrameBuffers();
	}
}

gfx::RenderTarget::~RenderTarget()
{
	auto logical_device = m_context->m_logical_device;

	// Clean depth buffer
	if (m_depth_buffer_view != VK_NULL_HANDLE) vkDestroyImageView(logical_device, m_depth_buffer_view, nullptr);
	if (m_depth_buffer != VK_NULL_HANDLE) vkDestroyImage(logical_device, m_depth_buffer, nullptr);
	if (m_depth_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(logical_device, m_depth_buffer_memory, nullptr);

	for (auto& view : m_image_views)
	{
		vkDestroyImageView(logical_device, view, nullptr);
	}

	for (auto& image : m_images)
	{
		if (image != VK_NULL_HANDLE) vkDestroyImage(logical_device, image, nullptr);
	}

	for (auto& image_memory : m_images_memory)
	{
		vkFreeMemory(logical_device, image_memory, nullptr);
	}

	vkDestroyRenderPass(logical_device, m_render_pass, nullptr);

	for (auto buffer : m_frame_buffers)
	{
		vkDestroyFramebuffer(logical_device, buffer, nullptr);
	}
}

void gfx::RenderTarget::Resize(std::uint32_t width, std::uint32_t height)
{
	auto logical_device = m_context->m_logical_device;
	m_desc.m_width = width;
	m_desc.m_height = height;

	// cleanup // TODO: Create cleanup function.

	// Clean depth buffer
	if (m_depth_buffer_view != VK_NULL_HANDLE) vkDestroyImageView(logical_device, m_depth_buffer_view, nullptr);
	if (m_depth_buffer != VK_NULL_HANDLE) vkDestroyImage(logical_device, m_depth_buffer, nullptr);
	if (m_depth_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(logical_device, m_depth_buffer_memory, nullptr);

	std::vector<VkImage> m_images;
	std::vector<VkDeviceMemory> m_images_memory;

	for (auto& image_view : m_image_views)
	{
		vkDestroyImageView(logical_device, image_view, nullptr);
	}
	m_image_views.clear();

	for (auto& image : m_images)
	{
		vkDestroyImage(logical_device, image, nullptr);
	}
	m_images.clear();

	for (auto& image_memory : m_images_memory)
	{
		vkFreeMemory(logical_device, image_memory, nullptr);
	}
	m_images_memory.clear();

	vkDestroyRenderPass(logical_device, m_render_pass, nullptr);

	for (auto buffer : m_frame_buffers)
	{
		vkDestroyFramebuffer(logical_device, buffer, nullptr);
	}
	m_frame_buffers.clear();

	// Recreate stuff TODO: This is all duplicate from the cosntructor
	CreateImages();
	CreateImageViews();

	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		CreateDepthBuffer();
		CreateDepthBufferView();
	}

	CreateRenderPass();

	CreateFrameBuffers();
}

std::uint32_t gfx::RenderTarget::GetWidth()
{
	return m_desc.m_width;
}

std::uint32_t gfx::RenderTarget::GetHeight()
{
	return m_desc.m_height;
}

void gfx::RenderTarget::CreateImages()
{
	auto num_rtvs = m_desc.m_rtv_formats.size();
	m_images_memory.resize(num_rtvs);
	m_images.resize(num_rtvs);

	auto logical_device = m_context->m_logical_device;

	for (std::size_t i = 0; i < m_desc.m_rtv_formats.size(); i++)
	{
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = m_desc.m_width;
		image_info.extent.height = m_desc.m_height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = m_desc.m_is_cube_map ? 6 : 1;
		image_info.format = m_desc.m_rtv_formats[i];
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // TODO: Optimize this
		if (m_desc.m_allow_uav || m_desc.m_allow_direct_access) image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.flags = m_desc.m_is_cube_map ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

		if (vkCreateImage(logical_device, &image_info, nullptr, &m_images[i]) != VK_SUCCESS)
		{
			LOGC("Failed to create texture");
		}

		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(logical_device, m_images[i], &memory_requirements);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = memory_requirements.size;
		alloc_info.memoryTypeIndex = m_context->FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(logical_device, &alloc_info, nullptr, &m_images_memory[i]) != VK_SUCCESS)
		{
			LOGC("failed to allocate image memory!");
		}
		VK_NAME_OBJ_DEF(logical_device, m_images_memory[i], VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)

		vkBindImageMemory(logical_device, m_images[i], m_images_memory[i], 0);
	}
}

void gfx::RenderTarget::CreateImageViews()
{
	auto logical_device = m_context->m_logical_device;

	m_image_views.resize(m_images.size());

	for (size_t i = 0; i < m_images.size(); i++) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_images[i];
		create_info.viewType = m_desc.m_is_cube_map ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = m_desc.m_rtv_formats[i];
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = m_desc.m_is_cube_map ? 6 : 1;;

		if (vkCreateImageView(logical_device, &create_info, nullptr, &m_image_views[i]) != VK_SUCCESS)
		{
			LOGC("failed to create image views!");
		}
	}
}

void gfx::RenderTarget::CreateFrameBuffers()
{
	auto logical_device = m_context->m_logical_device;

	m_frame_buffers.resize(1);

	std::vector<VkImageView> attachments = m_image_views;
	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		attachments.push_back(m_depth_buffer_view);
	}

	VkFramebufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	buffer_info.renderPass = m_render_pass;
	buffer_info.attachmentCount = attachments.size();
	buffer_info.pAttachments = attachments.data();
	buffer_info.width = m_desc.m_width;
	buffer_info.height = m_desc.m_height;
	buffer_info.layers = 1;

	if (vkCreateFramebuffer(logical_device, &buffer_info, nullptr, &m_frame_buffers[0]) != VK_SUCCESS)
	{
		LOGC("failed to create framebuffer!");
	}
}

void gfx::RenderTarget::CreateRenderPass()
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
		color_attachment.loadOp = m_desc.m_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		color_attachments.push_back(color_attachment);

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
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
		depth_attachment.loadOp = m_desc.m_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depth_attachment_ref.attachment = color_attachment_refs.size();
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	m_subpass = {};
	m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	m_subpass.colorAttachmentCount = color_attachment_refs.size();
	m_subpass.pColorAttachments = color_attachment_refs.data();
	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		m_subpass.pDepthStencilAttachment = &depth_attachment_ref;
	}

	std::vector<VkAttachmentDescription> depth_and_color_attachments = color_attachments;
	if (m_desc.m_depth_format != VK_FORMAT_UNDEFINED)
	{
		depth_and_color_attachments.push_back(depth_attachment);
	}

	m_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_render_pass_create_info.attachmentCount = depth_and_color_attachments.size();
	m_render_pass_create_info.pAttachments = depth_and_color_attachments.data();
	m_render_pass_create_info.subpassCount = 1;
	m_render_pass_create_info.pSubpasses = &m_subpass;
	m_render_pass_create_info.dependencyCount = dependencies.size();
	m_render_pass_create_info.pDependencies = dependencies.data();

	if (vkCreateRenderPass(logical_device, &m_render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		LOGC("failed to create render pass!");
	}
}

void gfx::RenderTarget::CreateDepthBuffer()
{
	auto logical_device = m_context->m_logical_device;

	// TODO: Check if we support the depth buffer format.
	m_depth_buffer_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	m_depth_buffer_create_info.imageType = VK_IMAGE_TYPE_2D;
	m_depth_buffer_create_info.extent.width = m_desc.m_width;
	m_depth_buffer_create_info.extent.height = m_desc.m_height;
	m_depth_buffer_create_info.extent.depth = 1;
	m_depth_buffer_create_info.mipLevels = 1;
	m_depth_buffer_create_info.arrayLayers = 1;
	m_depth_buffer_create_info.format = m_desc.m_depth_format;
	m_depth_buffer_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	m_depth_buffer_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_depth_buffer_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
	VK_NAME_OBJ_DEF(logical_device, m_depth_buffer_memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)

	vkBindImageMemory(logical_device, m_depth_buffer, m_depth_buffer_memory, 0);

	// Transition
}

void gfx::RenderTarget::CreateDepthBufferView()
{
	auto logical_device = m_context->m_logical_device;

	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = m_depth_buffer;
	view_create_info.viewType = m_desc.m_is_cube_map ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
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
