/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <optional>
#include <string>

namespace settings
{

	static const bool use_imgui = true;
	static const char* imgui_ini_filename = "editor_config.ini";
	static const std::optional<std::string> m_imgui_font = "fonts/DroidSans.ttf";
	static const std::optional<float> m_imgui_font_size = 13;
	static const bool use_multithreading = false;
	static const std::uint32_t num_frame_graph_threads = 4;

} /* settings */
