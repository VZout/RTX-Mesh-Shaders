/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "application.hpp"

#include <imgui.h>

#include "util/log.hpp"
#include "settings.hpp"
#include "imgui/IconsFontAwesome5.h"
#include "imgui/imgui_impl_glfw.hpp"

Application::Application(std::string const & name)
	: m_window(nullptr), m_name(name)
{
	if (!glfwInit())
	{
		LOGE("Failed to initialize GLFW");
	}
}

Application::~Application()
{
	Destroy();
}

void Application::KeyCallback_Internal(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scan_code, action, mods);
	static_cast<Application*>(glfwGetWindowUserPointer(window))->KeyCallback(key, action);
}

void Application::ResizeCallback_Internal(GLFWwindow* window, int width, int height)
{
	static_cast<Application*>(glfwGetWindowUserPointer(window))->ResizeCallback(static_cast<std::uint32_t>(width),
	                                                                            static_cast<std::uint32_t>(height));
}

void Application::MouseButtonCallback_Internal(GLFWwindow* window, int key, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, key, action, mods);
	static_cast<Application*>(glfwGetWindowUserPointer(window))->MouseButtonCallback(key, action);
}

void Application::MousePosCallback_Internal(GLFWwindow* window, double x, double y)
{
	static_cast<Application*>(glfwGetWindowUserPointer(window))->MousePosCallback(static_cast<float>(x), static_cast<float>(y));
}

void Application::CharCallback_Internal(GLFWwindow* window, unsigned int c)
{
	ImGui_ImplGlfw_CharCallback(window, c);
}

void Application::ScrollCallback_Internal(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void Application::Start(std::uint32_t width, std::uint32_t height)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(width, height, m_name.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		LOGC("Failed to create GLFW window.");
		glfwTerminate();
	}

	if (!glfwVulkanSupported())
	{
		LOGC("Vulkan is not supported!");
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, Application::KeyCallback_Internal);
	glfwSetFramebufferSizeCallback(m_window, Application::ResizeCallback_Internal);
	glfwSetMouseButtonCallback(m_window, Application::MouseButtonCallback_Internal);
	glfwSetCursorPosCallback(m_window, Application::MousePosCallback_Internal);
	glfwSetScrollCallback(m_window, Application::ScrollCallback_Internal);
	glfwSetCharCallback(m_window, Application::CharCallback_Internal);

	// Init ImGui
	if constexpr (settings::use_imgui)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		io.ConfigDockingWithShift = true;

		if (settings::m_imgui_font.has_value())
		{
			io.Fonts->AddFontFromFileTTF(settings::m_imgui_font.value().c_str(), settings::m_imgui_font_size.value_or(13.f));
		}

		// Enable TTF Icons
		{
			static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
			ImFontConfig icons_config;
			icons_config.MergeMode = true;
			icons_config.PixelSnapH = true;
			io.Fonts->AddFontFromFileTTF((std::string("fonts/") + std::string(FONT_ICON_FILE_NAME_FAS)).c_str(), settings::m_imgui_font_size.value_or(13.f), &icons_config, icons_ranges);
		}

		ImGui_ImplGlfw_InitForVulkan(m_window, false);
	}

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

bool Application::IsFullscreen() const
{
	return glfwGetWindowMonitor(m_window) != nullptr;
}

void Application::SetFullscreen(bool value)
{
	if (IsFullscreen() == value)
	{
		LOGW("Window is already in that mode. nothing is being done");
		return;
	}

	if (value)
	{
		// backup window position and window size
		glfwGetWindowPos(m_window, &prev_x, &prev_y);
		glfwGetWindowSize(m_window, &prev_width, &prev_height);

		// get resolution of monitor
		const auto monitor = glfwGetPrimaryMonitor();
		const auto mode = glfwGetVideoMode(monitor);

		// Switch to full screen
		glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, 0 );
	}
	else
	{
		// restore last window size and position
		glfwSetWindowMonitor(m_window, nullptr, prev_x, prev_y, prev_width, prev_height, 0 );
	}
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

void Application::SetMouseVisibility(bool value)
{
	glfwSetInputMode(m_window, GLFW_CURSOR, value ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Application::SetMousePos(float x, float y)
{
	glfwSetCursorPos(m_window, x, y);
}

HWND Application::GetNativeHandle()
{
	return glfwGetWin32Window(m_window);
}

GLFWwindow* Application::GetWindow()
{
	return m_window;
}

void Application::Destroy()
{
	if constexpr(settings::use_imgui)
	{
		ImGui_ImplGlfw_Shutdown();
	}

	glfwDestroyWindow(m_window);
	glfwTerminate();
}