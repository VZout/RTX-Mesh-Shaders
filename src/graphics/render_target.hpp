/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

class ImGuiImpl;

namespace gfx
{

	class Context;

	class RenderTarget
	{
		friend class PipelineState;
		friend class CommandList;
		friend class DescriptorHeap;
		friend class ::ImGuiImpl;
	public:
		struct Desc
		{
			std::uint32_t m_width;
			std::uint32_t m_height;
			std::vector<VkFormat> m_rtv_formats;
			VkFormat m_depth_format = VK_FORMAT_UNDEFINED;
		};

		explicit RenderTarget(Context* context);
		RenderTarget(Context* context, Desc desc);
		virtual ~RenderTarget();
		virtual void Resize(std::uint32_t width, std::uint32_t height);

		std::uint32_t GetWidth();
		std::uint32_t GetHeight();

	protected:
		void CreateImages();
		void CreateImageViews();
		void CreateFrameBuffers();
		void CreateRenderPass();
		void CreateDepthBuffer();
		void CreateDepthBufferView();

		Context* m_context;

		// Frame Buffers
		std::vector<VkImage> m_images;
		std::vector<VkDeviceMemory> m_images_memory;
		std::vector<VkImageView> m_image_views;
		std::vector<VkFramebuffer> m_frame_buffers;

		// Render pass
		VkSubpassDescription m_subpass;
		VkRenderPass m_render_pass;
		VkRenderPassCreateInfo m_render_pass_create_info;
		VkImageCreateInfo m_depth_buffer_create_info;
		VkImage m_depth_buffer;
		VkDeviceMemory m_depth_buffer_memory;
		VkImageView m_depth_buffer_view;
		Desc m_desc;
	};

} /* gfx */