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
#include <vk_mem_alloc.h>

class Application;
class Renderer;
class ImGuiImpl;

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
		friend class Shader;
		friend class PipelineState;
		friend class RootSignature;
		friend class RenderTarget;
		friend class CommandList;
		friend class Fence;
		friend class MemoryPool;
		friend class GPUBuffer;
		friend class StagingBuffer;
		friend class StagingTexture;
		friend class DescriptorHeap;
		friend class Texture;
		friend class VkMaterialPool;
		friend class VkConstantBufferPool;
		friend class AccelerationStructure;
		friend class ShaderTable;

		friend class ::ImGuiImpl; // TODO: probably dont want this
	public:
		Context(Application* app);
		~Context();

		std::vector<VkExtensionProperties> GetSupportedExtensions();
		std::vector<VkExtensionProperties> GetSupportedDeviceExtensions();
		VkPhysicalDeviceProperties2 GetPhysicalDeviceProperties();
		VkPhysicalDeviceRayTracingPropertiesNV GetRayTracingDeviceProperties();
		const VkPhysicalDeviceMemoryProperties* GetPhysicalDeviceMemoryProperties();
		
		bool HasValidationLayerSupport();
		std::uint32_t GetDirectQueueFamilyIdx();
		void WaitForDevice();
		std::uint32_t FindMemoryType(std::uint32_t filter, VkMemoryPropertyFlags properties);
		VmaStats CalculateVMAStats();

		inline static PFN_vkDebugMarkerSetObjectTagEXT DebugMarkerSetObjectTag = VK_NULL_HANDLE;
		inline static PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName = VK_NULL_HANDLE;
		inline static PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBegin = VK_NULL_HANDLE;
		inline static PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEnd = VK_NULL_HANDLE;
		inline static PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsert = VK_NULL_HANDLE;

		inline static PFN_vkCmdDrawMeshTasksNV CmdDrawMeshTasksNV = VK_NULL_HANDLE;

		inline static PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
		inline static PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
		inline static PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
		inline static PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
		inline static PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
		inline static PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
		inline static PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
		inline static PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
		inline static PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;

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
		void SetupDebugMarkerExtension();
		void SetupMeshShadingExtension();
		void SetupRayTracingExtension();
		void SetupVMA();

		VkApplicationInfo m_app_info = {};
		VkInstanceCreateInfo  m_instance_create_info;
		VkInstance m_instance;
		VkDebugUtilsMessengerCreateInfoEXT m_debug_messenger_create_info;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkDevice m_logical_device;
		VkPhysicalDevice m_physical_device;
		VkPhysicalDeviceFeatures2 m_physical_device_features;
		VkPhysicalDeviceProperties2 m_physical_device_properties;
		VkPhysicalDeviceRayTracingPropertiesNV m_physical_device_raytracing_properties;
		VkPhysicalDeviceMemoryProperties m_physical_device_mem_properties;
		QueueFamilyIndices m_queue_family_indices;
		SwapChainSupportDetails m_swapchain_support_details;
		VkSurfaceKHR m_surface;
		VmaAllocator m_vma_allocator;

		Application* m_app;
	};

} /* gfx */

