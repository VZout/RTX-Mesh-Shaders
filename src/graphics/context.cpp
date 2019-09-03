#include "context.hpp"

#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#include "gfx_settings.hpp"

namespace internal
{

	static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	static 	VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = internal::ValidationLayerCallback;
		create_info.pUserData = nullptr;

		return create_info;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, create_info, allocator, debug_messenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debug_messenger, allocator);
		}
	}

} /* internal */

gfx::Context::Context()
	: m_app_info(), m_instance_create_info(), m_instance(),
	m_debug_messenger(), m_debug_messenger_create_info(),
	m_physical_device(VK_NULL_HANDLE)
{
	std::uint32_t glfw_extension_count;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	// TODO: Check if the glfw extensions are supported.

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (gfx::settings::enable_validation_layers) {
		m_debug_messenger_create_info = internal::GetDebugMessengerCreateInfo();
		if (!HasValidationLayerSupport())
		{
			std::cout << "Requested validation layers but doesn't support it" << std::endl;
		}
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	m_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	m_app_info.pApplicationName = "Hello";
	m_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	m_app_info.pEngineName = "Vik Engine";
	m_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	m_app_info.apiVersion = VK_API_VERSION_1_1;

	m_instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	m_instance_create_info.pApplicationInfo = &m_app_info;
	m_instance_create_info.pNext = gfx::settings::enable_validation_layers ? &m_debug_messenger_create_info : nullptr;
	m_instance_create_info.enabledLayerCount = gfx::settings::enable_validation_layers ? gfx::settings::validation_layers.size() : 0;
	m_instance_create_info.ppEnabledLayerNames = gfx::settings::validation_layers.data();
	m_instance_create_info.enabledExtensionCount = extensions.size();
	m_instance_create_info.ppEnabledExtensionNames = extensions.data();

	auto result = vkCreateInstance(&m_instance_create_info, nullptr, &m_instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance.");
	}

	if (gfx::settings::enable_validation_layers)
	{
		EnableDebugCallback();
	}
}

gfx::Context::~Context()
{
	if (gfx::settings::enable_validation_layers) {
		internal::DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
	}

	vkDestroyInstance(m_instance, nullptr);
}

std::vector<VkExtensionProperties> gfx::Context::GetSupportedExtensions()
{
	std::uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

	return extensions;
}

bool gfx::Context::HasValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : gfx::settings::validation_layers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void gfx::Context::EnableDebugCallback()
{
	if (internal::CreateDebugUtilsMessengerEXT(m_instance, &m_debug_messenger_create_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}
