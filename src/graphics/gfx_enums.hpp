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

	enum class TextureFilter
	{
		FILTER_LINEAR,
		FILTER_POINT,
		FILTER_LINEAR_POINT,
		FILTER_ANISOTROPIC
	};

	enum class TextureAddressMode
	{
		TAM_MIRROR_ONCE = (int)VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
		TAM_MIRROR = (int)VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		TAM_CLAMP = (int)VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		TAM_BORDER = (int)VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		TAM_WRAP = (int)VK_SAMPLER_ADDRESS_MODE_REPEAT,
	};

	enum class BorderColor
	{
		BORDER_TRANSPARENT = (int)VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
		BORDER_BLACK = (int)VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		BORDER_WHITE = (int)VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
	};

	enum class BufferUsageFlag
	{
		INDEX_BUFFER = (int)VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VERTEX_BUFFER = (int)VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		CONSTANT_BUFFER = (int)VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		TRANSFER_SRC = (int)VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	};

	enum class PipelineType
	{
		COMPUTE_PIPE,
		GRAPHICS_PIPE
	};

	inline bool FormatHasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

}