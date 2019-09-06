/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "application.hpp"

Application::Application(std::string const & name)
	: m_window(nullptr), m_name(name)
{
	if (!glfwInit())
	{
		// Initialization failed
	}
}

Application::~Application()
{
	Destroy();
}

void Application::KeyCallback_Internal(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
	static_cast<Application*>(glfwGetWindowUserPointer(window))->KeyCallback(key, action);
}

void Application::Start(std::uint32_t width, std::uint32_t height)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	m_window = glfwCreateWindow(width, height, m_name.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		// Window or OpenGL context creation failed
		glfwTerminate();
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, Application::KeyCallback_Internal);

	Init();

	while (!glfwWindowShouldClose(m_window))
	{
		Loop();

		glfwPollEvents();
	}

	Destroy();
}

void Application::Close()
{
	glfwSetWindowShouldClose(m_window, true);
}

std::uint32_t Application::GetWidth() const
{
	std::int32_t width, _;
	glfwGetWindowSize(m_window, &width, &_);

	return static_cast<std::uint32_t>(width);
}

std::uint32_t Application::GetHeight() const
{
	std::int32_t height, _;
	glfwGetWindowSize(m_window, &_, &height);

	return static_cast<std::uint32_t>(height);
}

void Application::SetVisibility(bool value)
{
	if (value)
	{
		glfwShowWindow(m_window);
	}
	else
	{
		glfwHideWindow(m_window);
	}
}

HWND Application::GetNativeHandle()
{
	return glfwGetWin32Window(m_window);
}

void Application::Destroy()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}