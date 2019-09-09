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
		explicit RootSignature(Context* context);
		~RootSignature();
		void Compile();

	private:
		std::vector<VkDescriptorSetLayoutBinding> m_layout_bindings;
		std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
		VkPipelineLayoutCreateInfo m_pipeline_layout_info;
		VkPipelineLayout m_pipeline_layout;

		Context* m_context;
	};

} /* gfx */