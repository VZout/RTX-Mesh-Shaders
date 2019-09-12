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
		friend class ::ImGuiImpl;
	public:
		explicit RenderTarget(Context* context);
		virtual ~RenderTarget();

		std::uint32_t GetWidth();
		std::uint32_t GetHeight();

	protected:
		VkFormat PickDepthFormat();
		virtual void CreateRenderPass(VkFormat format, VkFormat depth_format = VK_FORMAT_UNDEFINED);
		void CreateDepthBuffer();
		void CreateDepthBufferView();

		Context* m_context;

		// Frame Buffers
		std::vector<VkFramebuffer> m_frame_buffers;

		// Render pass
		VkSubpassDescription m_subpass;
		VkRenderPass m_render_pass;
		VkRenderPassCreateInfo m_render_pass_create_info;
		VkImageCreateInfo m_depth_buffer_create_info;
		VkImage m_depth_buffer;
		VkDeviceMemory m_depth_buffer_memory;
		VkImageView m_depth_buffer_view;
		std::uint32_t m_width;
		std::uint32_t m_height;
	};

} /* gfx */