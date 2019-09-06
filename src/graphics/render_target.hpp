/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace gfx
{

	class Context;

	class RenderTarget
	{
		friend class PipelineState;
		friend class CommandList;
	public:
		explicit RenderTarget(Context* context);
		virtual ~RenderTarget();

		std::uint32_t GetWidth();
		std::uint32_t GetHeight();

	protected:
		void CreateRenderPass(VkFormat format);

		// Frame Buffers
		std::vector<VkFramebuffer> m_frame_buffers;

		// Render pass
		VkAttachmentDescription m_color_attachment;
		VkSubpassDescription m_subpass;
		VkRenderPass m_render_pass;
		VkRenderPassCreateInfo m_render_pass_create_info;
		std::uint32_t m_width;
		std::uint32_t m_height;

		Context* m_context;
	};

}; /* gfx */