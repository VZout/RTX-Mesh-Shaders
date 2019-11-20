/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vk_mem_alloc.h>
#include <optional>

#include "gfx_enums.hpp"

class ImGuiImpl;

namespace gfx
{

	class Context;

	class MemoryPool
	{
		friend class GPUBuffer;
		friend class Texture;
	public:
		MemoryPool(Context* context, std::size_t block_size, std::size_t num_blocks);
		~MemoryPool();

	private:
		Context* m_context;
		VmaPool m_pool;

		std::size_t m_block_size;
		std::size_t m_num_blocks;
	};

	class GPUBuffer
	{
		friend class CommandList;
		friend class DescriptorHeap;
		friend class ShaderTable;
		friend class ::ImGuiImpl;
		friend class VkConstantBufferPool;
	public:
		GPUBuffer(Context* context, std::optional<MemoryPool*> pool, std::uint64_t size);
		GPUBuffer(Context* context, std::optional<MemoryPool*> pool, std::uint64_t size, enums::BufferUsageFlag usage);
		GPUBuffer(Context* context, std::optional<MemoryPool*> pool, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage);
		virtual ~GPUBuffer();

		virtual void Map();
		virtual void Unmap();
		void Update(void* data, std::uint64_t size, std::uint64_t offset = 0);

	protected:
		void CreateBufferAndMemory(std::optional<MemoryPool*> pool, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
			VkBuffer& buffer, VmaAllocation& allocation);
		void Map_Internal(VmaAllocation& allocation);
		void Unmap_Internal(VmaAllocation& allocation);

		Context* m_context;
		std::optional<MemoryPool*> m_pool;

		std::uint64_t m_size;
		bool m_mapped;
		void* m_mapped_data;
		VkBuffer m_buffer;
		VmaAllocation m_buffer_allocation;
	};

	class StagingBuffer : public GPUBuffer
	{
		friend class CommandList;
	public:
		StagingBuffer(Context* context, std::optional<MemoryPool*> pool, std::optional<MemoryPool*> staging_pool, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage);
		~StagingBuffer() final;

		void Map() final;
		void Unmap() final;
		void FreeStagingResources();

	private:
		std::uint64_t m_stride;
		VkBuffer m_staging_buffer;
		VmaAllocation m_staging_buffer_allocation;
	};

	class Texture
	{
		friend class DescriptorHeap;
		friend class CommandList;
	public:
		struct Desc
		{
			VkFormat m_format;

			std::uint32_t m_width = 0u;
			std::uint32_t m_height = 0u;
			std::uint32_t m_channels = 0u;
			std::uint32_t m_depth = 1u;
			std::uint32_t m_array_size = 1u;
			std::uint32_t m_mip_levels = 1u;
			bool m_is_hdr = false;
		};

		Texture(Context* context, std::optional<MemoryPool*> pool, Desc desc);
		virtual ~Texture();

		bool HasMipMaps();

	protected:
		void CreateImageAndMemory(VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memory_usage,
		                          VkImage& image, VmaAllocation& allocation);
	private:
		Context* m_hidden_context;
		std::optional<MemoryPool*> m_hidden_pool;

	protected:
		Desc m_desc;
		VkImage m_texture;
		VmaAllocation m_texture_allocation;
	};

	class StagingTexture : public GPUBuffer, public Texture
	{
		friend class DescriptorHeap;
		friend class CommandList;
	public:

		StagingTexture(Context* context, std::optional<MemoryPool*> pool, Desc desc);
		StagingTexture(Context* context, std::optional<MemoryPool*> pool, Desc desc, void* pixels);
		~StagingTexture() final = default;

		void FreeStagingResources();

	private:
	};

} /* gfx */
