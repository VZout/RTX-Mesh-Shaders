#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace gfx
{

	class Context
	{
	public:
		Context();
		~Context();

		std::vector<VkExtensionProperties> GetSupportedExtensions();
		bool HasValidationLayerSupport();

	private:
		void EnableDebugCallback();

		VkApplicationInfo m_app_info = {};
		VkInstanceCreateInfo  m_instance_create_info;
		VkInstance m_instance;
		VkDebugUtilsMessengerCreateInfoEXT m_debug_messenger_create_info;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_physical_device;

	};

} /* gfx */

