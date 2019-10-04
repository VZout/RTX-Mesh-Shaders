/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */


#pragma once

#include <string>
#include <vector>
#include <optional>

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

	Editor();
	virtual ~Editor() = default;

	void RegisterCategory(std::string const & name, std::optional<icon_t> icon = std::nullopt);
	void RegisterAction(std::string const & name, std::string const & category, action_func_t action_func, std::optional<icon_t> icon = std::nullopt);
	void RegisterWindow(std::string const & name, std::string const & category, window_func_t window_func, bool default_visibility = false, std::optional<icon_t> icon = std::nullopt);
	void Render();

private:
	struct ActionDesc
	{
		std::string m_name;
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
		std::optional<icon_t> m_icon;
		std::vector<WindowDesc> m_window_descs;
		std::vector<ActionDesc> m_action_descs;
	};

	std::vector<CategoryDesc> m_category_descs;
	bool m_show_main_menu;

};
