/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 *  Thanks to `https://github.com/PixarAnimationStudios/` for the packing code.
 */

#pragma once

namespace util
{

	// https://github.com/PixarAnimationStudios/OpenSubdiv/blob/master/opensubdiv/far/patchParam.h#L234
	inline static std::uint32_t Pack(std::uint32_t value, int width, int offset)
	{
		return (std::uint32_t)((value & ((1 << width) - 1)) << offset);
	}

	// https://github.com/PixarAnimationStudios/OpenSubdiv/blob/master/opensubdiv/far/patchParam.h#L238
    inline static std::uint32_t Unpack(std::uint32_t value, int width, int offset)
	{
    	return (std::uint32_t)((value >> offset) & ((1 << width) - 1));
    }

} /* util */
