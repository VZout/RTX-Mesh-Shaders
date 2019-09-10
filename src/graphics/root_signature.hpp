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
		};

		RootSignature(Context* context, Desc desc);
		~RootSignature();
		void Compile();

	private:
		Desc m_desc;
		std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
		VkPipelineLayoutCreateInfo m_pipeline_layout_info;
		VkPipelineLayout m_pipeline_layout;

		Context* m_context;
	};

} /* gfx */