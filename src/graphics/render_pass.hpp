/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>

namespace gfx
{

	class Context;

	class RenderPass 
	{
	public:
		RenderPass(Context* context, VkFormat format);
		~RenderPass();
	
	private:
		VkAttachmentDescription m_color_attachment;
		VkSubpassDescription m_subpass;
		VkRenderPass m_render_pass;
		VkRenderPassCreateInfo m_render_pass_create_info;

		Context* m_context;
	};

} /* gfx */

