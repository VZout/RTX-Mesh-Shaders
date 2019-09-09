/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "viewport.hpp"

gfx::Viewport::Viewport(std::uint32_t width, std::uint32_t height)
	: m_viewport(), m_scissor()
{
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;
	m_viewport.width = static_cast<float>(width);
	m_viewport.height = static_cast<float>(height);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	m_scissor.offset = {0, 0};
	m_scissor.extent.width = width;
	m_scissor.extent.height = height;
}

void gfx::Viewport::Resize(std::uint32_t width, std::uint32_t height)
{
	m_viewport.width = static_cast<float>(width);
	m_viewport.height = static_cast<float>(height);

	m_scissor.extent.width = width;
	m_scissor.extent.height = height;
}