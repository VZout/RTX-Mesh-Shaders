/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "editor.hpp"
#include "imgui/imgui_style.hpp"

#include <util/log.hpp>

Editor::Editor()
	: m_show_main_menu(true), m_editor_visibility(true)
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

	LOGW("Failed to register action '{}'. No suitable category found.", name);
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

	LOGW("Failed to register window '{}'. No suitable category found.", name);
}

void Editor::RegisterModal(const std::string& name, modal_func_t modal_func)
{
	for (auto& modal_desc : m_modal_descs)
	{
		if (modal_desc.m_name == name)
		{
			LOGW("The modal '{}' is duplicate", name);
		}
	}

	m_modal_descs.emplace_back(ModalDesc{
		.m_name = name,
		.m_function = modal_func
	});
}

void Editor::Render()
{
	if (!m_editor_visibility) return;

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

		if (m_main_menu_bar_text.has_value())
		{
			ImGui::SameLine(ImGuiCol_Text, 15);
			ImGui::PushStyleColor(0, ImVec4(0.860f, 0.930f, 0.890f, 0.5f));
			ImGui::Text(m_main_menu_bar_text.value().c_str());
			ImGui::PopStyleColor();
		}

		ImGui::EndMainMenuBar();
	}

	// Prefer docking
	ImGui::DockSpaceOverViewport(true, ImGui::GetMainViewport());

	// Render the windows
	for (auto & cat_desc : m_category_descs)
	{
		for (auto & window_desc : cat_desc.m_window_descs)
		{
			if (window_desc.m_open)
			{
#ifdef IMGUI_SHOW_ICONS_IN_MENU_TITLES
				std::string window_name = fmt::format(window_desc.m_icon.has_value() ? "{0}  {1}" : "{1}", window_desc.m_icon.value_or("[erroricon]"), window_desc.m_name);
#else
				std::string window_name = window_desc.m_name;
#endif
				ImGui::Begin(window_name.c_str(), &window_desc.m_open);

				window_desc.m_function();

				ImGui::End();
			}
		}
	}

	// Open Modals that require opening
	for (auto const & name : m_open_modals)
	{
		ImGui::OpenPopup(name.c_str());
	}
	m_open_modals.clear();

	// Render modals
	for (auto const& modal_desc : m_modal_descs)
	{
		if (ImGui::BeginPopupModal(modal_desc.m_name.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
		{
			modal_desc.m_function();

			ImGui::EndPopup();
		}
	}
}

void Editor::OpenModal(std::string const& name)
{
	m_open_modals.push_back(name);
}

void Editor::SetTexture(ImTextureID texture)
{
	m_texture = texture;
}

void Editor::SetMainMenuBarText(std::string const& string)
{
	m_main_menu_bar_text = string;
}

void Editor::SetEditorVisibility(bool value)
{
	m_editor_visibility = value;
}

bool Editor::GetEditorVisibility() const
{
	return m_editor_visibility;
}

ImTextureID Editor::GetTexture()
{
	return m_texture;
}
