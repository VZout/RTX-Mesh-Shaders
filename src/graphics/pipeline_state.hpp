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
	class Viewport;
	class RootSignature;

	class PipelineState
	{
	public:
		explicit PipelineState(Context* context);
		~PipelineState();

		void SetViewport(Viewport* viewport);
		void SetRootSignature(RootSignature* root_signature);

		void Compile();

	private:
		VkPipelineVertexInputStateCreateInfo m_vertex_input_info;
		VkPipelineInputAssemblyStateCreateInfo m_ia_info;
		VkPipelineViewportStateCreateInfo m_viewport_info;
		VkPipelineRasterizationStateCreateInfo m_raster_info;
		VkPipelineMultisampleStateCreateInfo m_ms_info;
		VkPipelineColorBlendAttachmentState m_color_blend_attachment_info;
		VkPipelineColorBlendStateCreateInfo m_color_blend_info;
		VkPipelineLayout m_layout;
		RootSignature* m_root_signature;

		Context* m_context;
	};

} /* gfx */
