/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "render_target.hpp"

#include <cstdint>

class ImGuiImpl;

namespace gfx
{

	class Context;
	class CommandQueue;
	class Fence;

	class RenderWindow : public RenderTarget
	{
		friend class ::ImGuiImpl;
	public:
		RenderWindow(Context*  context);
		~RenderWindow() final;

		void AquireBackBuffer(Fence* fence);
		void Present(CommandQueue* queue, Fence* fence);
		void Resize(std::uint32_t width, std::uint32_t height);
		std::uint32_t GetFrameIdx();

	private:
		VkSurfaceFormatKHR PickSurfaceFormat();
		VkPresentModeKHR PickPresentMode();
		VkExtent2D ComputeSwapchainExtend();
		std::uint32_t ComputeNumBackBuffers();
		void CreateSwapchain();
		void GetSwapchainImages();
		void CreateSwapchainImageViews();
		void CreateSwapchainFrameBuffers();

		std::uint32_t m_frame_idx;
		VkSwapchainCreateInfoKHR m_swapchain_create_info;
		VkSwapchainKHR m_swapchain;
	};

} /* gfx */