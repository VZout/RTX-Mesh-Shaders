/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Application
{
public:
	explicit Application(std::string const & name);
	virtual ~Application();

	void Create(std::uint32_t width, std::uint32_t height);
	void Start(std::uint32_t width, std::uint32_t height);
	void Close();
	[[nodiscard]] std::uint32_t GetWidth() const;
	[[nodiscard]] std::uint32_t GetHeight() const;
	bool IsFullscreen() const;
	void SetFullscreen(bool value);
	void SetVisibility(bool value);
	void SetMouseVisibility(bool value);
	void SetMousePos(float x, float y);
	GLFWwindow* GetWindow();

protected:
	virtual void Init() = 0;
	virtual void Loop() = 0;

	virtual void ResizeCallback([[maybe_unused]] std::uint32_t width, [[maybe_unused]] std::uint32_t height) { /* Do nothing if its not overriden */ };
	virtual void KeyCallback([[maybe_unused]] int key, [[maybe_unused]] int action) { /* Do nothing if its not overridden */ };
	virtual void MouseButtonCallback([[maybe_unused]] int key, [[maybe_unused]] int action) { /* Do nothing if its not overriden */ }
	virtual void MousePosCallback([[maybe_unused]] float x, [[maybe_unused]] float y) { /* Do nothing if its not overriden */ }

	static void KeyCallback_Internal(GLFWwindow* window, int key, int scan_code, int action, int mods);
	static void ResizeCallback_Internal(GLFWwindow* window, int width, int height);
	static void MouseButtonCallback_Internal(GLFWwindow* window, int key, int action, int mods);
	static void MousePosCallback_Internal(GLFWwindow* window, double x, double y);
	static void CharCallback_Internal(GLFWwindow* window, unsigned int c);
	static void ScrollCallback_Internal(GLFWwindow* window, double xoffset, double yoffset);

	void Destroy();

	GLFWwindow* m_window;
	const std::string m_name;

private:
	bool m_created;
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
