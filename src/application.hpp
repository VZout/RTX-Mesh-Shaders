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
	bool IsFullscreen() const;
	void SetFullscreen(bool value);
	void SetVisibility(bool value);
	HWND GetNativeHandle();
	GLFWwindow* GetWindow();

protected:
	virtual void Init() = 0;
	virtual void Loop() = 0;

	virtual void ResizeCallback([[maybe_unused]] std::uint32_t width, [[maybe_unused]] std::uint32_t height) { /* Do nothing if its not overriden */ };
	virtual void KeyCallback([[maybe_unused]] int key, [[maybe_unused]] int action) { /* Do nothing if its not overridden */ };

	static void KeyCallback_Internal(GLFWwindow* window, int key, int scan_code, int action, int mods);
	static void ResizeCallback_Internal(GLFWwindow* window, int width, int height);

	void Destroy();

	GLFWwindow* m_window;
	const std::string m_name;

private:
	int prev_x, prev_y, prev_width, prev_height;

};

#define ENTRY(AppType, width, height) \
int main()                            \
{                                     \
	auto app = new AppType();         \
	app->Start(width, height);        \
	delete app;                       \
	return 0;                         \
}
