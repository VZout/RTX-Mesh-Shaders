/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>
#include "vulkan/vulkan.h"

namespace gfx::settings
{
	static const bool enable_validation_layers = true;
	static const std::vector<const char*> validation_layers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
	static const std::vector<const char*> instance_extensions =
	{
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};
	static const std::vector<const char*> device_extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_NV_MESH_SHADER_EXTENSION_NAME,
		VK_NV_RAY_TRACING_EXTENSION_NAME,
		VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
	};
	static const std::uint32_t num_back_buffers = 2;
	static const VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
	static const VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	static const VkColorSpaceKHR swapchain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	static const VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
	static const std::uint32_t max_lights = 50;
	static const std::uint32_t max_render_batch_size = 100;
}
