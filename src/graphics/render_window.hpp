/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "vulkan/vulkan.h"

#include <cstdint>
#include <vector>

namespace gfx
{

	class Context;
	class CommandQueue;

	class RenderWindow
	{
	public:
		RenderWindow(Context*  context);
		~RenderWindow();

		void Present(CommandQueue* queue);

	private:
		VkSurfaceFormatKHR PickSurfaceFormat();
		VkPresentModeKHR PickPresentMode();
		VkExtent2D ComputeSwapchainExtend();
		std::uint32_t ComputeNumBackBuffers();
		void GetSwapchainImages();
		void CreateSwapchainImageViews();

		std::uint32_t m_frame_idx;
		VkSwapchainKHR m_swapchain;
		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;

		Context* m_context;
	};

} /* gfx */