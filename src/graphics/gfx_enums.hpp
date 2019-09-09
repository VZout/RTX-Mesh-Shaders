/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>
#include "vulkan/vulkan.h"

namespace gfx::enums
{

	enum class BufferUsageFlag
	{
		INDEX_BUFFER = (int)VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VERTEX_BUFFER = (int)VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		CONSTANT_BUFFER = (int)VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	};

}