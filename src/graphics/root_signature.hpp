/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace gfx
{

	class Context;

	class RootSignature
	{
		friend class PipelineState;
		friend class CommandList;
		friend class DescriptorHeap;
	public:
		struct Desc
		{
			std::vector<VkDescriptorSetLayoutBinding> m_parameters;
			std::vector<VkPushConstantRange> m_push_constants = {};
		};

		RootSignature(Context* context, Desc desc);
		~RootSignature();
		void Compile();

	private:
		Context* m_context;

		Desc m_desc;
		std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
		VkPipelineLayout m_pipeline_layout;
		VkPipelineLayoutCreateInfo m_pipeline_layout_info;

	};

} /* gfx */