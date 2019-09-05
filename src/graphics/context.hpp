/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <optional>

class Application;

namespace gfx
{
	struct QueueFamilyIndices {
		std::optional<std::uint32_t> direct_family;

		bool HasDirectFamily();
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR m_capabilities;
		std::vector<VkSurfaceFormatKHR> m_formats;
		std::vector<VkPresentModeKHR> m_present_modes;
	};

	class Context
	{
		friend class CommandQueue;
		friend class RenderWindow;
	public:
		Context(Application* app);
		~Context();

		std::vector<VkExtensionProperties> GetSupportedExtensions();
		std::vector<VkExtensionProperties> GetSupportedDeviceExtensions();
		bool HasValidationLayerSupport();
		std::uint32_t GetDirectQueueFamilyIdx();

	private:
		std::vector<VkExtensionProperties> GetSupportedDeviceExtensions(VkPhysicalDevice device);
		void CreateSurface();
		void CreateLogicalDevice();
		VkPhysicalDevice FindPhysicalDevice();
		bool HasExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails GetSwapChainSupportDetails(VkPhysicalDevice device);
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
		SwapChainSupportDetails m_swapchain_support_details;
		VkSurfaceKHR m_surface;
		VkWin32SurfaceCreateInfoKHR m_surface_create_info;

		Application* m_app;
	};

} /* gfx */
