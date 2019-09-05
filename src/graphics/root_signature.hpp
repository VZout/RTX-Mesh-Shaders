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

	class RootSignature
	{
	public:
		explicit RootSignature(Context* context);
		~RootSignature();
		void Compile();

	private:
		VkPipelineLayoutCreateInfo m_pipeline_layout_info;
		VkPipelineLayout m_pipeline_layout;

		Context* m_context;
	};

} /* gfx */