/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "acceleration_structure.hpp"

#include "context.hpp"
#include "command_list.hpp"
#include "gpu_buffers.hpp"
#include "../util/log.hpp"
#include "gfx_defines.hpp"

gfx::AccelerationStructure::AccelerationStructure(Context* context)
	: m_context(context)
{

}

gfx::AccelerationStructure::~AccelerationStructure()
{
	auto logical_device = m_context->m_logical_device;

	vkFreeMemory(logical_device, m_memory, nullptr);
	Context::vkDestroyAccelerationStructureNV(logical_device, m_native, nullptr);

	DestroyScratchResources();
}

void gfx::AccelerationStructure::CreateTopLevel(CommandList* cmd_list, std::vector<InstanceDesc> instance_descs)
{
	auto logical_device = m_context->m_logical_device;

	VkAccelerationStructureInfoNV geom_info = {};
	geom_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	geom_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	geom_info.instanceCount = instance_descs.size();
	geom_info.geometryCount = 0;
	geom_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;

	// actual geometry instances, this is clearer in dx12 imo
	std::vector<InstanceDesc_Internal> geom_instances = {};
	for (auto & desc : instance_descs)
	{
		InstanceDesc_Internal int_desc;
		int_desc.mask = 0xff;
		int_desc.instanceCustomIndex = desc.m_material;
		int_desc.instanceOffset = 0;
		int_desc.flags = desc.m_two_sided ? VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV /*| VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_NV*/ : 0;
		int_desc.transform = desc.m_transform;
		int_desc.accelerationStructureHandle = desc.m_blas->GetHandle();
		geom_instances.push_back(int_desc);
	}

	CreateAS(geom_info);

	AllocateAndBindMemory();

	PopupateHandle();

	CreateScratchBuffer();

	CreateInstanceBuffer(geom_instances);

	auto n_cmd_list = cmd_list->m_cmd_buffers[cmd_list->m_frame_idx];

	Context::vkCmdBuildAccelerationStructureNV(
		n_cmd_list,
		&geom_info,
		m_instance_buffer->m_buffer,
		0,
		VK_FALSE,
		m_native,
		VK_NULL_HANDLE,
		m_scratch_buffer->m_buffer,
		0);

	VK_NAME_OBJ(logical_device, m_native, VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT, "Top Level Acceleration Structure");

	Barrier(n_cmd_list);
}

void gfx::AccelerationStructure::CreateBottomLevel(CommandList* cmd_list, std::vector<GeometryDesc> geometry_descs)
{
	auto logical_device = m_context->m_logical_device;

	std::vector<VkGeometryNV> geometries;
	geometries.reserve(geometry_descs.size());
	for (auto desc : geometry_descs)
	{
		if (!desc.m_vb || !desc.m_ib)
		{
			LOGW("Invalid geometry description");
		}

		glm::mat3x4 zero_transform = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
		};

		VkGeometryNV geom = {};
		geom.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geom.geometryType = VkGeometryTypeNV::VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geom.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geom.geometry.triangles.vertexData = desc.m_vb->m_buffer;
		geom.geometry.triangles.vertexOffset = desc.m_vertices_offset;
		geom.geometry.triangles.vertexCount = desc.m_num_vertices;
		geom.geometry.triangles.vertexStride = desc.m_vertex_stride;
		geom.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geom.geometry.triangles.indexData = desc.m_ib->m_buffer;
		geom.geometry.triangles.indexOffset = desc.m_indices_offset;
		geom.geometry.triangles.indexCount = desc.m_num_indices;
		geom.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geom.geometry.triangles.transformData = VK_NULL_HANDLE;
		geom.geometry.triangles.transformOffset = 0;
		geom.geometry.aabbs = {};
		geom.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
		geom.flags = 0; // VK_GEOMETRY_OPAQUE_BIT_NV

		geometries.push_back(geom);
	}

	VkAccelerationStructureInfoNV geom_info = {};
	geom_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	geom_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	geom_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
	geom_info.instanceCount = 0;
	geom_info.geometryCount = geometries.size();
	geom_info.pGeometries = geometries.data();

	CreateAS(geom_info);

	AllocateAndBindMemory();

	PopupateHandle();

	CreateScratchBuffer();

	auto n_cmd_list = cmd_list->m_cmd_buffers[cmd_list->m_frame_idx];

	VkAccelerationStructureMemoryRequirementsInfoNV mem_requirements_info = {};
	mem_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	mem_requirements_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	mem_requirements_info.pNext = nullptr;
	mem_requirements_info.accelerationStructure = m_native;

	VkMemoryRequirements2 scratch_memory_requirements;
	Context::vkGetAccelerationStructureMemoryRequirementsNV(logical_device, &mem_requirements_info, &scratch_memory_requirements);

	Context::vkCmdBuildAccelerationStructureNV(
		n_cmd_list,
		&geom_info,
		VK_NULL_HANDLE,
		0,
		VK_FALSE,
		m_native,
		VK_NULL_HANDLE,
		m_scratch_buffer->m_buffer,
		0);

	VK_NAME_OBJ(logical_device, m_native, VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT, "Bottom Level Acceleration Structure");

	Barrier(n_cmd_list);
}

void gfx::AccelerationStructure::DestroyScratchResources()
{
	if (m_scratch_buffer) delete m_scratch_buffer;
	if (m_instance_buffer) delete m_instance_buffer;
}

std::uint64_t gfx::AccelerationStructure::GetHandle()
{
	return m_handle;
}

void gfx::AccelerationStructure::CreateAS(VkAccelerationStructureInfoNV const& info)
{
	auto logical_device = m_context->m_logical_device;

	VkAccelerationStructureCreateInfoNV create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	create_info.pNext = nullptr;
	create_info.info = info;
	if (Context::vkCreateAccelerationStructureNV(logical_device, &create_info, nullptr, &m_native) != VK_SUCCESS)
	{
		LOGE("Failed to create bottom level acceleration structure");
	}
}

void gfx::AccelerationStructure::CreateScratchBuffer()
{
	auto logical_device = m_context->m_logical_device;

	VkAccelerationStructureMemoryRequirementsInfoNV mem_requirements_info = {};
	mem_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	mem_requirements_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	mem_requirements_info.pNext = nullptr;
	mem_requirements_info.accelerationStructure = m_native;

	VkMemoryRequirements2 scratch_memory_requirements;
	Context::vkGetAccelerationStructureMemoryRequirementsNV(logical_device, &mem_requirements_info, &scratch_memory_requirements);

	const auto scratch_size = scratch_memory_requirements.memoryRequirements.size;

	m_scratch_buffer = new GPUBuffer(m_context, std::nullopt, scratch_size, gfx::enums::BufferUsageFlag::RAYTRACING);
}

void gfx::AccelerationStructure::CreateInstanceBuffer(std::vector<InstanceDesc_Internal> & instance_descs)
{
	auto logical_device = m_context->m_logical_device;

	m_instance_buffer = new GPUBuffer(m_context, std::nullopt,
		instance_descs.data(), instance_descs.size(), sizeof(InstanceDesc_Internal),
		gfx::enums::BufferUsageFlag::RAYTRACING, VMA_MEMORY_USAGE_CPU_ONLY);
}

void gfx::AccelerationStructure::AllocateAndBindMemory()
{
	auto logical_device = m_context->m_logical_device;

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_native;

	VkMemoryRequirements2 mem_requirements{};
	Context::vkGetAccelerationStructureMemoryRequirementsNV(logical_device, &memoryRequirementsInfo, &mem_requirements);

	VkMemoryAllocateInfo mem_alloc_info = {};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.allocationSize = mem_requirements.memoryRequirements.size;
	mem_alloc_info.memoryTypeIndex = m_context->FindMemoryType(mem_requirements.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(logical_device, &mem_alloc_info, nullptr, &m_memory) != VK_SUCCESS)
	{
		LOGE("Failed to allocate memory for acceleration structure");
	}

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_native;
	accelerationStructureMemoryInfo.memory = m_memory;
	if (Context::vkBindAccelerationStructureMemoryNV(logical_device, 1, &accelerationStructureMemoryInfo) != VK_SUCCESS)
	{
		LOGE("Failed to bind bottom level acceleration structure");
	}
}

void gfx::AccelerationStructure::PopupateHandle()
{
	auto logical_device = m_context->m_logical_device;

	if (Context::vkGetAccelerationStructureHandleNV(logical_device, m_native, sizeof(uint64_t), &m_handle) != VK_SUCCESS)
	{
		LOGE("Failed to get bottom level acceleration structure handle");
	}
}

void gfx::AccelerationStructure::Barrier(VkCommandBuffer n_cmd_list)
{
	VkMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier(n_cmd_list, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &barrier, 0, 0, 0, 0);
}

void gfx::AccelerationStructure::SetScratchBuffer(GPUBuffer* buffer)
{
	m_scratch_buffer = buffer;
}
