/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "gpu_buffers.hpp"

#include "../util/log.hpp"
#include "context.hpp"
#include "gfx_defines.hpp"

template<typename T, typename A>
constexpr inline T SizeAlignTwoPower(T size, A alignment)
{
	return (size + (alignment - 1U)) & ~(alignment - 1U);
}

template<typename T, typename A>
constexpr inline T SizeAlignAnyAlignment(T size, A alignment)
{
	return (size / alignment + (size % alignment > 0)) * alignment;
}

gfx::MemoryPool::MemoryPool(Context* context, std::size_t block_size, std::size_t num_blocks)
	: m_context(context), m_block_size(block_size), m_num_blocks(num_blocks)
{
	auto vma_allocator = context->m_vma_allocator;

	VkBufferCreateInfo exampleBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	exampleBufCreateInfo.size = block_size;
	exampleBufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	uint32_t memTypeIndex;
	vmaFindMemoryTypeIndexForBufferInfo(vma_allocator, &exampleBufCreateInfo, &allocCreateInfo, &memTypeIndex);

	VmaPoolCreateInfo pool_create_info = {};
	pool_create_info.memoryTypeIndex = memTypeIndex;
	pool_create_info.blockSize = SizeAlignAnyAlignment(block_size, 256);
	pool_create_info.maxBlockCount = num_blocks;
	pool_create_info.flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT;

	if (vmaCreatePool(vma_allocator, &pool_create_info, &m_pool) != VK_SUCCESS)
	{
		LOGC("Failed to create VMA memory pool");
	}
}

gfx::MemoryPool::~MemoryPool()
{
	auto vma_allocator = m_context->m_vma_allocator;

	vmaDestroyPool(vma_allocator, m_pool);
}

gfx::GPUBuffer::GPUBuffer(gfx::Context* context, std::optional<MemoryPool*> pool, std::uint64_t size)
	: m_context(context), m_pool(pool), m_size(size), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_allocation(VK_NULL_HANDLE)
{
}

gfx::GPUBuffer::GPUBuffer(Context* context, std::optional<MemoryPool*> pool, std::uint64_t size, enums::BufferUsageFlag usage)
	: m_context(context), m_pool(pool), m_size(size), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_allocation(VK_NULL_HANDLE)
{
	// Create default buffer
	CreateBufferAndMemory(m_pool, m_size, (int)usage, VMA_MEMORY_USAGE_CPU_TO_GPU,
	                      m_buffer, m_buffer_allocation);
}

gfx::GPUBuffer::GPUBuffer(Context* context, std::optional<MemoryPool*> pool, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage)
	: m_context(context), m_pool(pool), m_size(size * stride), m_mapped(false), m_mapped_data(nullptr), m_buffer(VK_NULL_HANDLE),
	m_buffer_allocation(VK_NULL_HANDLE)
{
	// Create default buffer
	CreateBufferAndMemory(m_pool, m_size, (int)usage, VMA_MEMORY_USAGE_CPU_TO_GPU,
	                      m_buffer, m_buffer_allocation);

	Map();
	Update(data, m_size);
	Unmap();
}

gfx::GPUBuffer::~GPUBuffer()
{
	if (m_mapped)
	{
		Unmap();
	}

	auto vma_allocator = m_context->m_vma_allocator;

	if (m_buffer != VK_NULL_HANDLE) vmaDestroyBuffer(m_context->m_vma_allocator, m_buffer, m_buffer_allocation);
}

void gfx::GPUBuffer::Map()
{
	Map_Internal(m_buffer_allocation);
}

void gfx::GPUBuffer::Unmap()
{
	Unmap_Internal(m_buffer_allocation);
}

void gfx::GPUBuffer::Update(void* data, std::uint64_t size, std::uint64_t offset)
{
	if (!m_mapped)
	{
		LOGC("Can't update a buffer that is not mapped.");
	}

	memcpy(static_cast<std::uint8_t*>(m_mapped_data) + offset, data, static_cast<std::size_t>(size));
}

void gfx::GPUBuffer::CreateBufferAndMemory(std::optional<MemoryPool*> pool, VkDeviceSize size, VkBufferUsageFlags usage,
	VmaMemoryUsage memory_usage, VkBuffer& buffer, VmaAllocation& allocation)
{
	// Create the buffer.
	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.usage = memory_usage;
	alloc_create_info.pool = pool.has_value() ? pool.value()->m_pool : VK_NULL_HANDLE;

	if (pool.has_value())
	{
		if (pool.value()->m_block_size != size)
		{
			LOGE("Mismatch between pool size and buffer size");
		}
	}

	VkResult r = vmaCreateBuffer(m_context->m_vma_allocator, &buffer_create_info, &alloc_create_info, &buffer, &allocation, nullptr);
	if (r != VK_SUCCESS) {
		LOGC("Failed to allocate VMA buffer");
	}
}

void gfx::GPUBuffer::Map_Internal(VmaAllocation& allocation)
{
	auto vma_allocator = m_context->m_vma_allocator;

	if (vmaMapMemory(vma_allocator, allocation, &m_mapped_data) != VK_SUCCESS)
	{
		LOGC("Failed to map vma allocation");
	}

	m_mapped = true;
}

void gfx::GPUBuffer::Unmap_Internal(VmaAllocation& allocation)
{
	auto vma_allocator = m_context->m_vma_allocator;

	vmaUnmapMemory(vma_allocator, allocation);

	m_mapped = false;
}

gfx::StagingBuffer::StagingBuffer(Context* context, std::optional<MemoryPool*> pool, std::optional<MemoryPool*> staging_pool, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage)
	: GPUBuffer(context, pool, size * stride), m_staging_buffer(VK_NULL_HANDLE), m_staging_buffer_allocation(VK_NULL_HANDLE)
{
	m_stride = stride;

	// Create staging buffer
	CreateBufferAndMemory(pool, m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	                      m_staging_buffer, m_staging_buffer_allocation);

	Map();
	Update(data, m_size);
	Unmap();

	// Create default buffer TODO: remove buffer usage
	CreateBufferAndMemory(staging_pool, m_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | (int)usage, VMA_MEMORY_USAGE_GPU_ONLY,
	                      m_buffer, m_buffer_allocation);
}

gfx::StagingBuffer::~StagingBuffer()
{
	auto vma_allocator = m_context->m_vma_allocator;

	if (m_staging_buffer != VK_NULL_HANDLE) vmaDestroyBuffer(vma_allocator, m_staging_buffer, m_staging_buffer_allocation);
}

void gfx::StagingBuffer::Map()
{
	Map_Internal(m_staging_buffer_allocation);
}

void gfx::StagingBuffer::Unmap()
{
	Unmap_Internal(m_staging_buffer_allocation);
}

void gfx::StagingBuffer::FreeStagingResources()
{
	auto vma_allocator = m_context->m_vma_allocator;

	vmaDestroyBuffer(vma_allocator, m_staging_buffer, m_staging_buffer_allocation);
	m_staging_buffer = VK_NULL_HANDLE;
	m_staging_buffer_allocation = VK_NULL_HANDLE;
}


gfx::Texture::Texture(gfx::Context* context, std::optional<MemoryPool*> pool, gfx::Texture::Desc desc)
		: m_hidden_context(context), m_hidden_pool(pool), m_desc(desc),	m_texture(VK_NULL_HANDLE), m_texture_allocation(VK_NULL_HANDLE)
{
}

gfx::Texture::~Texture()
{
	auto vma_allocator = m_hidden_context->m_vma_allocator;

	if (m_texture != VK_NULL_HANDLE) vmaDestroyImage(vma_allocator, m_texture, m_texture_allocation);
}

bool gfx::Texture::HasMipMaps()
{
	return m_desc.m_mip_levels > 1;
}

void gfx::Texture::CreateImageAndMemory(VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memory_usage,
										VkImage& image, VmaAllocation& allocation)
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

	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.usage = memory_usage;
	alloc_create_info.pool = m_hidden_pool.has_value() ? m_hidden_pool.value()->m_pool : VK_NULL_HANDLE;

	if (vmaCreateImage(m_hidden_context->m_vma_allocator, &image_info, &alloc_create_info, &image, &allocation, nullptr) != VK_SUCCESS)
	{
		LOGC("Failed to allocate VMA image");
	}
}

gfx::StagingTexture::StagingTexture(Context* context, std::optional<MemoryPool*> pool, Desc desc)
		: GPUBuffer(context, pool, desc.m_width * desc.m_height * enums::BytesPerPixel(desc.m_format)), Texture(context, pool, desc)
{
	CreateBufferAndMemory(pool, m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	                      m_buffer, m_buffer_allocation);

	CreateImageAndMemory(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                     VMA_MEMORY_USAGE_GPU_ONLY, m_texture, m_texture_allocation);
}

gfx::StagingTexture::StagingTexture(Context* context, std::optional<MemoryPool*> pool, Desc desc, void* pixels)
		: GPUBuffer(context, pool, desc.m_width * desc.m_height * enums::BytesPerPixel(desc.m_format)), Texture(context, pool, desc)
{
	CreateBufferAndMemory(pool, m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	                      m_buffer, m_buffer_allocation);

	CreateImageAndMemory(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                     VMA_MEMORY_USAGE_GPU_ONLY, m_texture, m_texture_allocation);

	Map();
	Update(pixels, m_size);
	Unmap();
}

void gfx::StagingTexture::FreeStagingResources()
{
	auto vma_allocator = m_context->m_vma_allocator;

	if (m_buffer != VK_NULL_HANDLE) vmaDestroyBuffer(m_context->m_vma_allocator, m_buffer, m_buffer_allocation);

	m_buffer = VK_NULL_HANDLE;
	m_buffer_allocation = VK_NULL_HANDLE;
}