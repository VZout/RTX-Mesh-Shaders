/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

#include "render_target.hpp"
#include "viewport.hpp"

namespace gfx
{

	class Context;
	class Viewport;
	class RootSignature;
	class Shader;

	class PipelineState
	{
		friend class CommandList;
	public:
		struct Desc
		{
			std::vector<VkFormat> m_rtv_formats;
			VkFormat m_depth_format = VK_FORMAT_UNDEFINED;
			VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			bool m_counter_clockwise = true;
		};

		using InputLayout = std::pair<std::vector<VkVertexInputBindingDescription>, std::vector<VkVertexInputAttributeDescription>>;

		PipelineState(Context* context, Desc desc);
		~PipelineState();

		void SetViewport(Viewport* viewport);
		void SetRootSignature(RootSignature* root_signature);
		void AddShader(Shader* shader);
		void SetInputLayout(InputLayout const & input_layout);

		void Compile();
		void Recompile();

	private:
		void CreateRenderPass();

		Context* m_context;
		Desc m_desc;

		std::vector<VkPipelineShaderStageCreateInfo> m_shader_info;
		VkPipelineVertexInputStateCreateInfo m_vertex_input_info;
		VkPipelineInputAssemblyStateCreateInfo m_ia_info;
		VkPipelineViewportStateCreateInfo m_viewport_info;
		VkPipelineRasterizationStateCreateInfo m_raster_info;
		VkPipelineMultisampleStateCreateInfo m_ms_info;
		VkPipelineDepthStencilStateCreateInfo m_depth_stencil_info;
		std::vector<VkPipelineColorBlendAttachmentState> m_color_blend_attachment_info;
		VkPipelineColorBlendStateCreateInfo m_color_blend_info;
		VkPipelineLayout m_layout;
		RootSignature* m_root_signature;
		VkGraphicsPipelineCreateInfo m_create_info;
		VkPipeline m_pipeline;
		VkRenderPass m_render_pass;
		std::optional<InputLayout> m_input_layout;
	};

} /* gfx */
