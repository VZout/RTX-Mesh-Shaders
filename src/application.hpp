/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <string>

class Application
{
public:
	explicit Application(std::string const & name);
	virtual ~Application();

	void Start(std::uint32_t width, std::uint32_t height);
	void Close();
	[[nodiscard]] std::uint32_t GetWidth() const;
	[[nodiscard]] std::uint32_t GetHeight() const;
	void SetVisibility(bool value);
	HWND GetNativeHandle();

protected:
	virtual void Init() = 0;
	virtual void Loop() = 0;

	virtual void KeyCallback(int key, int action) { /* Do nothing if its not overridden */ };

	static void KeyCallback_Internal(GLFWwindow* window, int key, int scan_code, int action, int mods);

	void Destroy();

	GLFWwindow* m_window;
	const std::string m_name;
};

#define ENTRY(AppType, width, height) \
int main()                            \
{                                     \
	auto app = new AppType();         \
	app->Start(width, height);        \
	delete app;                       \
	return 0;                         \
}
