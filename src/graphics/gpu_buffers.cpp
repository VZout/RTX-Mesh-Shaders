/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "gpu_buffers.hpp"

#include "../util/log.hpp"
#include "context.hpp"

gfx::GPUBuffer::GPUBuffer(gfx::Context* context, std::uint64_t size)
	: m_context(context), m_size(size), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_memory(VK_NULL_HANDLE)
{

}

gfx::GPUBuffer::GPUBuffer(Context* context, std::uint64_t size, enums::BufferUsageFlag usage)
		: m_context(context), m_size(size), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
		  m_buffer_memory(VK_NULL_HANDLE)
{
	auto logical_device = m_context->m_logical_device;

	// Create default buffer
	CreateBufferAndMemory(m_size, (int)usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                      m_buffer, m_buffer_memory);
}

gfx::GPUBuffer::GPUBuffer(Context* context, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage)
	: m_context(context), m_size(size * stride), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_memory(VK_NULL_HANDLE)
{
	auto logical_device = m_context->m_logical_device;

	// Create default buffer
	CreateBufferAndMemory(m_size, (int)usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                      m_buffer, m_buffer_memory);

	Map();
	Update(data, m_size);
	Unmap();
}

gfx::GPUBuffer::~GPUBuffer()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyBuffer(logical_device, m_buffer, nullptr);
	vkFreeMemory(logical_device, m_buffer_memory, nullptr);
}

void gfx::GPUBuffer::Map()
{
	Map_Internal(m_buffer_memory);
}

void gfx::GPUBuffer::Unmap()
{
	Unmap_Internal(m_buffer_memory);
}

void gfx::GPUBuffer::Update(void* data, std::uint64_t size)
{
	if (!m_mapped)
	{
		LOGC("Can't update a buffer that is not mapped.");
	}

	memcpy(m_mapped_data, data, static_cast<std::size_t>(size));
}

void gfx::GPUBuffer::CreateBufferAndMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                           VkBuffer& buffer, VkDeviceMemory& memory)
{
	auto logical_device = m_context->m_logical_device;

	// Create the buffer.
	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
	{
		LOGC("failed to create vertex buffer!");
	}

	// Get buffer memory requirements
	VkMemoryRequirements buffer_memory_requirements;
	vkGetBufferMemoryRequirements(logical_device, buffer, &buffer_memory_requirements);

	// Allocate memory
	VkMemoryAllocateInfo buffer_alloc_info = {};
	buffer_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	buffer_alloc_info.allocationSize = buffer_memory_requirements.size;
	buffer_alloc_info.memoryTypeIndex = m_context->FindMemoryType(buffer_memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logical_device, &buffer_alloc_info, nullptr, &memory) != VK_SUCCESS)
	{
		LOGC("failed to allocate vertex buffer memory!");
	}

	// Bind the buffer to the memory allocation
	if (vkBindBufferMemory(logical_device, buffer, memory, 0))
	{
		LOGC("Failed to map buffer to memory.");
	}
}

void gfx::GPUBuffer::Map_Internal(VkDeviceMemory memory)
{
	auto logical_device = m_context->m_logical_device;

	if (vkMapMemory(logical_device, memory, 0, m_size, 0, &m_mapped_data) != VK_SUCCESS)
	{
		LOGC("Failed to map staging buffer");
	}

	m_mapped = true;
}

void gfx::GPUBuffer::Unmap_Internal(VkDeviceMemory memory)
{
	auto logical_device = m_context->m_logical_device;

	vkUnmapMemory(logical_device, memory);

	m_mapped = false;
}

gfx::StagingBuffer::StagingBuffer(Context* context, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage)
	: GPUBuffer(context, size * stride), m_staging_buffer(VK_NULL_HANDLE), m_staging_buffer_memory(VK_NULL_HANDLE)
{
	m_size = size * stride;

	auto logical_device = m_context->m_logical_device;

	// Create staging buffer
	CreateBufferAndMemory(m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                      m_staging_buffer, m_staging_buffer_memory);

	Map();
	Update(data, m_size);
	Unmap();

	// Create default buffer
	CreateBufferAndMemory(m_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | (int)usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                      m_buffer, m_buffer_memory);
}

gfx::StagingBuffer::~StagingBuffer()
{
	auto logical_device = m_context->m_logical_device;

	if (m_staging_buffer) vkDestroyBuffer(logical_device, m_staging_buffer, nullptr);
	if (m_staging_buffer_memory) vkFreeMemory(logical_device, m_staging_buffer_memory, nullptr);
}

void gfx::StagingBuffer::Map()
{
	Map_Internal(m_staging_buffer_memory);
}

void gfx::StagingBuffer::Unmap()
{
	Unmap_Internal(m_staging_buffer_memory);
}

void gfx::StagingBuffer::FreeStagingResources()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyBuffer(logical_device, m_staging_buffer, nullptr);
	vkFreeMemory(logical_device, m_staging_buffer_memory, nullptr);
	m_staging_buffer = VK_NULL_HANDLE;
	m_staging_buffer_memory = VK_NULL_HANDLE;
}
