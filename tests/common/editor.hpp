/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define IMGUI_SHOW_ICONS_IN_MENU_TITLES

#include <string>
#include <vector>
#include <optional>
#include <imgui.h>

#include <util/delegate.hpp>

enum class EditorWindowCategory
{
	FILE,
	TOOLS,
};

class Editor
{
public:
	using icon_t = const char*;
	using action_func_t = util::Delegate<void()>;
	using window_func_t = util::Delegate<void()>;
	using modal_func_t = util::Delegate<void()>;

	Editor();
	virtual ~Editor() = default;

	void RegisterCategory(std::string const & name, std::optional<icon_t> icon = std::nullopt,
		std::optional<std::string> sub_category = std::nullopt);
	void RegisterAction(std::string const & name, std::string const & category, action_func_t action_func,
		std::optional<icon_t> icon = std::nullopt,
		std::optional<std::string> sub_category = std::nullopt);
	void RegisterWindow(std::string const & name, std::string const & category, window_func_t window_func, bool default_visibility = false,
		std::optional<icon_t> icon = std::nullopt);
	void RegisterModal(std::string const & name, modal_func_t modal_func);
	void Render();
	void OpenModal(std::string const & name);
	void SetTexture(ImTextureID texture);
	void SetMainMenuBarText(std::string const & string);
	void SetEditorVisibility(bool value);
	bool GetEditorVisibility() const;
	ImTextureID GetTexture();

private:
	struct ActionDesc
	{
		std::string m_name;
		std::optional<std::string> m_sub_category;
		std::optional<icon_t> m_icon;
		action_func_t m_function;
	};

	struct WindowDesc
	{
		std::string m_name;
		std::optional<icon_t> m_icon;
		window_func_t m_function;
		bool m_open;
	};

	struct CategoryDesc
	{
		std::string m_name;
		std::optional<std::string> m_sub_category;
		std::optional<icon_t> m_icon;
		std::vector<WindowDesc> m_window_descs;
		std::vector<ActionDesc> m_action_descs;
	};

	struct ModalDesc
	{
		std::string m_name;
		modal_func_t m_function;
	};

	std::vector<CategoryDesc> m_category_descs;
	std::vector<ModalDesc> m_modal_descs;
	std::vector<std::string> m_open_modals;
	bool m_show_main_menu;
	bool m_editor_visibility;
	ImTextureID m_texture;
	std::optional<std::string> m_main_menu_bar_text;

};
