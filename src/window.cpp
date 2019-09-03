#include "window.hpp"

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

	m_window = glfwCreateWindow(width, height, m_name.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		// Window or OpenGL context creation failed
		glfwTerminate();
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, Application::KeyCallback_Internal);

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
	}

	Destroy();
}

void Application::Close()
{
	glfwSetWindowShouldClose(m_window, true);
}

void Application::Destroy()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}