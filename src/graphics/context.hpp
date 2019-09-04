#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <optional>

namespace gfx
{
	struct QueueFamilyIndices {
		std::optional<std::uint32_t> graphics_family;

		bool HasGraphicsFamily();
	};

	class Context
	{
		friend class CommandQueue;
	public:
		Context();
		~Context();

		std::vector<VkExtensionProperties> GetSupportedExtensions();
		bool HasValidationLayerSupport();
		std::uint32_t GetGraphicsQueueFamilyIdx();

	private:
		void CreateLogicalDevice();
		VkPhysicalDevice FindPhysicalDevice();
		std::uint32_t GetDeviceSuitabilityRating(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		void EnableDebugCallback();

		VkApplicationInfo m_app_info = {};
		VkInstanceCreateInfo  m_instance_create_info;
		VkInstance m_instance;
		VkDebugUtilsMessengerCreateInfoEXT m_debug_messenger_create_info;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkDevice m_logical_device;
		VkPhysicalDevice m_physical_device;
		VkPhysicalDeviceFeatures m_physical_device_features;
		VkPhysicalDeviceProperties m_physical_device_properties;
		QueueFamilyIndices m_queue_family_indices;
	};

} /* gfx */

