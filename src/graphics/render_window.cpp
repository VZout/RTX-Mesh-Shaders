/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "render_window.hpp"

#include <array>

#include "../application.hpp"
#include "../util/log.hpp"
#include "context.hpp"
#include "command_queue.hpp"
#include "fence.hpp"
#include "gfx_settings.hpp"

gfx::RenderWindow::RenderWindow(Context* context)
	: RenderTarget(context), m_frame_idx(0), m_swapchain_create_info()
{
	auto app = context->m_app;

	m_desc.m_depth_format = VK_FORMAT_D32_SFLOAT;
	m_desc.m_width = app->GetWidth();
	m_desc.m_height = app->GetHeight();
	m_desc.m_rtv_formats = { gfx::settings::swapchain_format };

	CreateSwapchain();

	GetSwapchainImages();
	CreateSwapchainImageViews();

	CreateDepthBuffer();
	CreateDepthBufferView();

	CreateRenderPass();

	CreateSwapchainFrameBuffers();
}

gfx::RenderWindow::~RenderWindow()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroySwapchainKHR(logical_device, m_swapchain, nullptr);

	// DestroySwapchainKHR destroys the images.
	for (auto& image : m_images)
	{
		image = VK_NULL_HANDLE;
	}
}


void gfx::RenderWindow::AquireBackBuffer(Fence* fence)
{
	auto logical_device = m_context->m_logical_device;

	std::uint32_t new_frame_idx = 0;
	vkAcquireNextImageKHR(logical_device, m_swapchain, UINT64_MAX, fence->m_wait_semaphore, VK_NULL_HANDLE, &new_frame_idx);

	if (m_frame_idx != new_frame_idx)
	{
		LOGE("Render window frame index is out of sync with the swap chain!");
	}
}

void gfx::RenderWindow::Present(CommandQueue* queue, Fence* fence)
{
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &fence->m_signal_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;
	present_info.pResults = nullptr;
	present_info.pImageIndices = &m_frame_idx;

	vkQueuePresentKHR(queue->m_queue, &present_info);

	m_frame_idx = (m_frame_idx + 1) % gfx::settings::num_back_buffers;
}

void gfx::RenderWindow::Resize(std::uint32_t width, std::uint32_t height)
{
	// Cleanup
	auto logical_device = m_context->m_logical_device;

	vkDestroyRenderPass(logical_device, m_render_pass, nullptr);

	for (auto& frame_buffer : m_frame_buffers) {
		vkDestroyFramebuffer(logical_device, frame_buffer, nullptr);
	}
	m_frame_buffers.clear();

	for (auto& view : m_image_views) {
		vkDestroyImageView(logical_device, view, nullptr);
	}
	m_images.clear();
	m_image_views.clear();

	// Clean depth buffer
	vkDestroyImageView(logical_device, m_depth_buffer_view, nullptr);
	vkDestroyImage(logical_device, m_depth_buffer, nullptr);
	vkFreeMemory(logical_device, m_depth_buffer_memory, nullptr);

	vkDestroySwapchainKHR(logical_device, m_swapchain, nullptr);

	m_desc.m_width = width;
	m_desc.m_height = height;

	// Recreate
	CreateSwapchain();

	GetSwapchainImages();
	CreateSwapchainImageViews();

	CreateDepthBuffer();
	CreateDepthBufferView();

	CreateRenderPass();

	CreateSwapchainFrameBuffers();

	m_frame_idx = 0;
}

std::uint32_t gfx::RenderWindow::GetFrameIdx()
{
	return m_frame_idx;
}

VkSurfaceFormatKHR gfx::RenderWindow::PickSurfaceFormat()
{
	auto available_formats = m_context->m_swapchain_support_details.m_formats;

	for (const auto& format : available_formats)
	{
		if (format.format == gfx::settings::swapchain_format && format.colorSpace == gfx::settings::swapchain_color_space)
		{
			return format;
		}
	}

	LOGC("Can't create swapchain with unsuported format or unsupported color space");
}

VkPresentModeKHR gfx::RenderWindow::PickPresentMode()
{
	auto available_present_modes =  m_context->m_swapchain_support_details.m_present_modes;

	for (const auto& present_mode : available_present_modes)
	{
		if (present_mode == gfx::settings::swapchain_present_mode)
		{
			return present_mode;
		}
	}

	LOGC("Can't create swapchain with unsuported present mode");
}

VkExtent2D gfx::RenderWindow::ComputeSwapchainExtend()
{
	auto capabilities = m_context->m_swapchain_support_details.m_capabilities;
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		auto app = m_context->m_app;
		VkExtent2D actual_extent = { app->GetWidth(), app->GetHeight() };

		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

std::uint32_t gfx::RenderWindow::ComputeNumBackBuffers()
{
	auto capabilities = m_context->m_swapchain_support_details.m_capabilities;

	if (gfx::settings::num_back_buffers < capabilities.minImageCount || gfx::settings::num_back_buffers > capabilities.maxImageCount)
	{
		LOGW("Invalid number of back buffers.");
	}

	return gfx::settings::num_back_buffers;
}

void gfx::RenderWindow::CreateSwapchain()
{
	auto surface_format = PickSurfaceFormat();
	auto present_mode = PickPresentMode();
	auto num_back_buffers = ComputeNumBackBuffers();

	auto capabilities = m_context->m_swapchain_support_details.m_capabilities;

	m_swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	m_swapchain_create_info.surface = m_context->m_surface;
	m_swapchain_create_info.minImageCount = num_back_buffers;
	m_swapchain_create_info.imageFormat = surface_format.format;
	m_swapchain_create_info.imageColorSpace = surface_format.colorSpace;
	m_swapchain_create_info.imageExtent.width = m_desc.m_width;
	m_swapchain_create_info.imageExtent.height = m_desc.m_height;
	m_swapchain_create_info.imageArrayLayers = 1;
	m_swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	m_swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	m_swapchain_create_info.queueFamilyIndexCount = 0;
	m_swapchain_create_info.pQueueFamilyIndices = nullptr;
	m_swapchain_create_info.preTransform = capabilities.currentTransform;
	m_swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	m_swapchain_create_info.presentMode = present_mode;
	m_swapchain_create_info.clipped = VK_TRUE;
	m_swapchain_create_info.oldSwapchain = VK_NULL_HANDLE; // TODO: Do i want this for rebuild?

	if (vkCreateSwapchainKHR(m_context->m_logical_device, &m_swapchain_create_info, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		LOGC("failed to create swap chain!");
	}
}

void gfx::RenderWindow::GetSwapchainImages()
{
	auto logical_device = m_context->m_logical_device;

	auto num_back_buffers = 0u;
	vkGetSwapchainImagesKHR(logical_device, m_swapchain, &num_back_buffers, nullptr);
	m_images.resize(num_back_buffers);
	vkGetSwapchainImagesKHR(logical_device, m_swapchain, &num_back_buffers, m_images.data());
}


void gfx::RenderWindow::CreateSwapchainImageViews()
{
	auto logical_device = m_context->m_logical_device;

	m_image_views.resize(gfx::settings::num_back_buffers);

	for (size_t i = 0; i < gfx::settings::num_back_buffers; i++) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = gfx::settings::swapchain_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logical_device, &create_info, nullptr, &m_image_views[i]) != VK_SUCCESS)
		{
			LOGC("failed to create image views!");
		}
	}
}

void gfx::RenderWindow::CreateSwapchainFrameBuffers()
{
	auto logical_device = m_context->m_logical_device;

	m_frame_buffers.resize(m_image_views.size());

	for (std::size_t i = 0; i < m_image_views.size(); i++)
	{
		std::vector<VkImageView> attachments =
		{
				m_image_views[i],
				m_depth_buffer_view
		};

		VkFramebufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		buffer_info.renderPass = m_render_pass;
		buffer_info.attachmentCount = attachments.size();
		buffer_info.pAttachments = attachments.data();
		buffer_info.width = m_desc.m_width;
		buffer_info.height = m_desc.m_height;
		buffer_info.layers = 1;

		if (vkCreateFramebuffer(logical_device, &buffer_info, nullptr, &m_frame_buffers[i]) != VK_SUCCESS)
		{
			LOGC("failed to create framebuffer!");
		}
	}
}
