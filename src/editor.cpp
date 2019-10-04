/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "editor.hpp"

#include <imgui.h>

#include "util/log.hpp"

Editor::Editor()
	: m_show_main_menu(true)
{

}

void Editor::RegisterCategory(std::string const& name, std::optional<icon_t> icon)
{
	m_category_descs.emplace_back(CategoryDesc{
		.m_name = name,
		.m_icon = icon,
	});
}

void Editor::RegisterAction(const std::string& name, std::string const & category, action_func_t action_func, std::optional<icon_t> icon)
{
	for (auto& cat_desc : m_category_descs)
	{
		if (cat_desc.m_name == category)
		{
			cat_desc.m_action_descs.emplace_back(ActionDesc{
				.m_name = name,
				.m_icon = icon,
				.m_function = action_func,
			});

			return;
		}
	}

	LOGW("Failed to register action {}. No suitable category found.", name);
}

void Editor::RegisterWindow(const std::string& name, std::string const & category, window_func_t window_func, bool default_visibility, std::optional<icon_t> icon)
{
	for (auto& cat_desc : m_category_descs)
	{
		if (cat_desc.m_name == category)
		{
			cat_desc.m_window_descs.emplace_back(WindowDesc{
				.m_name = name,
				.m_icon = icon,
				.m_function = window_func,
				.m_open = default_visibility
			});

			return;
		}
	}

	LOGW("Failed to register window {}. No suitable category found.", name);
}

void Editor::Render()
{
	// Render the menu bar
	if (m_show_main_menu && ImGui::BeginMainMenuBar())
	{
		for (auto& cat_desc : m_category_descs)
		{
			std::string cat_name = fmt::format(cat_desc.m_icon.has_value() ? "{0}  {1}" : "{1}", cat_desc.m_icon.value_or("[erroricon]"), cat_desc.m_name);
			if (ImGui::BeginMenu(cat_name.c_str()))
			{
				for (auto& action_desc : cat_desc.m_action_descs)
				{
					std::string action_name = fmt::format(action_desc.m_icon.has_value() ? "{0}  {1}" : "{1}", action_desc.m_icon.value_or("[erroricon]"), action_desc.m_name);
					if (ImGui::MenuItem(action_name.c_str(), nullptr))
					{
						action_desc.m_function();
					}
				}

				if (!cat_desc.m_action_descs.empty() && !cat_desc.m_window_descs.empty())
				{
					ImGui::Separator();
				}

				for (auto& window_desc : cat_desc.m_window_descs)
				{
					std::string window_name = fmt::format(window_desc.m_icon.has_value() ? "{0}  {1}" : "{1}", window_desc.m_icon.value_or("[erroricon]"), window_desc.m_name);
					ImGui::MenuItem(window_name.c_str(), nullptr, &window_desc.m_open);
				}

				ImGui::EndMenu();
			}
		}

		ImGui::EndMainMenuBar();
	}

	// Render the windows
	for (auto& cat_desc : m_category_descs)
	{
		for (auto& window_desc : cat_desc.m_window_descs)
		{
			if (window_desc.m_open)
			{
				ImGui::Begin(window_desc.m_name.c_str(), &window_desc.m_open);

				window_desc.m_function();

				ImGui::End();
			}
		}
	}

}
