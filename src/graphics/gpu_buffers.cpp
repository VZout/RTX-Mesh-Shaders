/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "gpu_buffers.hpp"

#include "../util/log.hpp"
#include "context.hpp"
#include "gfx_defines.hpp"

gfx::GPUBuffer::GPUBuffer(gfx::Context* context, std::uint64_t size)
	: m_context(context), m_size(size), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_memory(VK_NULL_HANDLE)
{

}

gfx::GPUBuffer::GPUBuffer(Context* context, std::uint64_t size, enums::BufferUsageFlag usage)
		: m_context(context), m_size(size), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
		  m_buffer_memory(VK_NULL_HANDLE)
{
	// Create default buffer
	CreateBufferAndMemory(m_size, (int)usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                      m_buffer, m_buffer_memory);
}

gfx::GPUBuffer::GPUBuffer(Context* context, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage)
	: m_context(context), m_size(size * stride), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_memory(VK_NULL_HANDLE)
{
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

	if (m_buffer != VK_NULL_HANDLE) vkDestroyBuffer(logical_device, m_buffer, nullptr);
	if (m_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(logical_device, m_buffer_memory, nullptr);
}

void gfx::GPUBuffer::Map()
{
	Map_Internal(m_buffer_memory);
}

void gfx::GPUBuffer::Unmap()
{
	Unmap_Internal(m_buffer_memory);
}

void gfx::GPUBuffer::Update(void* data, std::uint64_t size, std::uint64_t offset)
{
	if (!m_mapped)
	{
		LOGC("Can't update a buffer that is not mapped.");
	}

	memcpy(static_cast<std::uint8_t*>(m_mapped_data) + offset, data, static_cast<std::size_t>(size));
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
	VK_NAME_OBJ_DEF(logical_device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)

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
	m_stride = stride;

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


gfx::Texture::Texture(gfx::Context* context, gfx::Texture::Desc desc, bool uav)
		: m_hidden_context(context), m_desc(desc),	m_texture(VK_NULL_HANDLE), m_texture_memory(VK_NULL_HANDLE)
{
	if (uav)
	{
		CreateImageAndMemory(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture, m_texture_memory);
	}
}

gfx::Texture::~Texture()
{
	auto logical_device = m_hidden_context->m_logical_device;

	if (m_texture) vkDestroyImage(logical_device, m_texture, nullptr);
	if (m_texture_memory) vkFreeMemory(logical_device, m_texture_memory, nullptr);
}

bool gfx::Texture::HasMipMaps()
{
	return m_desc.m_mip_levels > 1;
}

void gfx::Texture::CreateImageAndMemory(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                        VkImage& image, VkDeviceMemory memory)
{
	auto logical_device = m_hidden_context->m_logical_device;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = m_desc.m_width;
	image_info.extent.height = m_desc.m_height;
	image_info.extent.depth = m_desc.m_depth;
	image_info.mipLevels = m_desc.m_mip_levels;
	image_info.arrayLayers = m_desc.m_array_size;
	image_info.format = m_desc.m_format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	if (m_desc.m_mip_levels > 1) image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	if (vkCreateImage(logical_device, &image_info, nullptr, &image) != VK_SUCCESS)
	{
		LOGC("Failed to create texture");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device, image, &memory_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = m_hidden_context->FindMemoryType(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logical_device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
	{
		LOGC("failed to allocate image memory!");
	}
	VK_NAME_OBJ_DEF(logical_device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)

	vkBindImageMemory(logical_device, image, memory, 0);
}

gfx::StagingTexture::StagingTexture(Context* context, Desc desc)
		: GPUBuffer(context, desc.m_width * desc.m_height * enums::BytesPerPixel(desc.m_format)), Texture(context, desc)
{
	CreateBufferAndMemory(m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                      m_buffer, m_buffer_memory);

	CreateImageAndMemory(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture, m_texture_memory);
}

gfx::StagingTexture::StagingTexture(Context* context, Desc desc, void* pixels)
		: GPUBuffer(context, desc.m_width * desc.m_height * enums::BytesPerPixel(desc.m_format)), Texture(context, desc)
{
	CreateBufferAndMemory(m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                      m_buffer, m_buffer_memory);

	CreateImageAndMemory(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture, m_texture_memory);

	Map();
	Update(pixels, m_size);
	Unmap();
}

void gfx::StagingTexture::FreeStagingResources()
{
	auto logical_device = m_context->m_logical_device;

	if (m_buffer != VK_NULL_HANDLE) vkDestroyBuffer(logical_device, m_buffer, nullptr);
	if (m_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(logical_device, m_buffer_memory, nullptr);
	m_buffer = VK_NULL_HANDLE;
	m_buffer_memory = VK_NULL_HANDLE;
}