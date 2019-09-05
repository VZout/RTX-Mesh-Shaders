/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace gfx
{

	class Viewport
	{
		friend class PipelineState;
	public:
		Viewport(std::uint32_t width, std::uint32_t height);
		~Viewport() = default;

	private:
		VkViewport m_viewport;
		VkRect2D m_scissor;
	};

} /* gfx */