/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>

#define VK_NAME_OBJ_DEF(device, vk_object, type) \
if (Context::DebugMarkerSetObjectName) { \
std::string full_name = std::string(__FILE__) + ":" + std::to_string(__LINE__); \
VkDebugMarkerObjectNameInfoEXT name_info = {}; \
name_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT; \
name_info.objectType = type; \
name_info.object = (std::uint64_t)vk_object; \
name_info.pObjectName = full_name.c_str(); \
Context::DebugMarkerSetObjectName(device, &name_info); \
}

#define VK_NAME_OBJ(device, vk_object, type, name) \
if (Context::DebugMarkerSetObjectName) { \
std::string full_name = std::string(__FILE__) + " - " + std::string(name); \
VkDebugMarkerObjectNameInfoEXT name_info = {}; \
name_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT; \
name_info.objectType = type; \
name_info.object = (std::uint64_t)vk_object; \
name_info.pObjectName = full_name.c_str(); \
Context::DebugMarkerSetObjectName(device, &name_info); \
}
