/*! A constexpr way to obtain program version information.
 *
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <glm.hpp>

constexpr float operator"" _deg(long double deg)
{
	return glm::radians(static_cast<float>(deg));
}

constexpr float operator"" _rad(long double rad)
{
	return glm::degrees(static_cast<float>(rad));
}
