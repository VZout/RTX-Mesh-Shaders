/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace gfx
{

	class Context;
	class CommandList;
	class GPUBuffer;
	class StagingBuffer;

	struct GeometryDesc
	{
		GPUBuffer* m_vb = nullptr;
		GPUBuffer* m_ib = nullptr;

		std::uint32_t m_num_vertices = 0u;
		std::uint32_t m_num_indices = 0u;
		std::uint32_t m_vertices_offset = 0u;
		std::uint32_t m_indices_offset = 0u;
		std::uint32_t m_vertex_stride = 0u;
	};
	
	class AccelerationStructure;

	struct InstanceDesc
	{
		AccelerationStructure* m_blas;
		glm::mat3x4 m_transform;
		std::uint32_t m_material;
	};

	struct InstanceDesc_Internal {
		glm::mat3x4			transform;
		std::uint32_t       instanceCustomIndex : 24;
		std::uint32_t       mask : 8;
		std::uint32_t       instanceOffset : 24;
		std::uint32_t       flags : 8;
		std::uint64_t       accelerationStructureHandle;
	};

	class AccelerationStructure
	{
		friend class DescriptorHeap;
	public:
		AccelerationStructure(Context* context);
		virtual ~AccelerationStructure();

		void CreateTopLevel(CommandList* cmd_list, std::vector<InstanceDesc> instance_descs);
		void CreateBottomLevel(CommandList* cmd_list, std::vector<GeometryDesc> geometry_descs);
		void DestroyScratchResources();
		std::uint64_t GetHandle();
		void SetScratchBuffer(GPUBuffer* buffer);

	private:
		void CreateAS(VkAccelerationStructureInfoNV const & info);
		void CreateScratchBuffer();
		void CreateInstanceBuffer(std::vector<InstanceDesc_Internal> & instance_descs);
		void AllocateAndBindMemory();
		void PopupateHandle();
		void Barrier(VkCommandBuffer n_cmd_list);


		GPUBuffer* m_scratch_buffer = nullptr;
		GPUBuffer* m_instance_buffer = nullptr;

		VkDeviceMemory m_memory;
		VkAccelerationStructureNV m_native;
		std::uint64_t m_handle;

		Context* m_context;
	};

} /* gfx */
