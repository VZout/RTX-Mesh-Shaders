/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "render_window.hpp"

#include <array>

#include "../application.hpp"
#include "context.hpp"
#include "command_queue.hpp"
#include "gfx_settings.hpp"

gfx::RenderWindow::RenderWindow(Context* context)
	: m_context(context)
{
	auto surface_format = PickSurfaceFormat();
	auto present_mode = PickPresentMode();
	auto extent = ComputeSwapchainExtend();
	auto num_back_buffers = ComputeNumBackBuffers();

	auto capabilities = m_context->m_swapchain_support_details.m_capabilities;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_context->m_surface;
	create_info.minImageCount = num_back_buffers;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;
	create_info.preTransform = capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_context->m_logical_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	GetSwapchainImages();
	CreateSwapchainImageViews();
}

gfx::RenderWindow::~RenderWindow()
{
	auto logical_device = m_context->m_logical_device;

	for (auto& view : m_swapchain_image_views) {
		vkDestroyImageView(logical_device, view, nullptr);
	}
	vkDestroySwapchainKHR(logical_device, m_swapchain, nullptr);
}

void gfx::RenderWindow::Present(CommandQueue* queue)
{
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 0;
	present_info.pWaitSemaphores = nullptr;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;
	present_info.pResults = nullptr;
	present_info.pImageIndices = &m_frame_idx;

	vkQueuePresentKHR(queue->m_queue, &present_info);
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

	throw std::runtime_error("Can't create swapchain with unsuported format or unsupported color space");
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

	throw std::runtime_error("Can't create swapchain with unsuported present mode");
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
		throw std::runtime_error("Invalid number of back buffers");
	}

	return gfx::settings::num_back_buffers;
}

void gfx::RenderWindow::GetSwapchainImages()
{
	auto logical_device = m_context->m_logical_device;

	auto num_back_buffers = 0u;
	vkGetSwapchainImagesKHR(logical_device, m_swapchain, &num_back_buffers, nullptr);
	m_swapchain_images.resize(num_back_buffers);
	vkGetSwapchainImagesKHR(logical_device, m_swapchain, &num_back_buffers, m_swapchain_images.data());
}


void gfx::RenderWindow::CreateSwapchainImageViews()
{
	auto logical_device = m_context->m_logical_device;

	m_swapchain_image_views.resize(gfx::settings::num_back_buffers);

	for (size_t i = 0; i < gfx::settings::num_back_buffers; i++) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_swapchain_images[i];
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

		if (vkCreateImageView(logical_device, &create_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}