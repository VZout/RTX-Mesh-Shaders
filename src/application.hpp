#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Application
{
public:
	explicit Application(std::string const & name);
	virtual ~Application();

	void Start(std::uint32_t width, std::uint32_t height);
	void Close();

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
int main() \
{ \
	auto app = new AppType(); \
	app->Start(width, height); \
	return 0; \
}
