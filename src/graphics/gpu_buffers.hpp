/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

#include "gfx_enums.hpp"

class ImGuiImpl;

namespace gfx
{

	class Context;

	class GPUBuffer
	{
		friend class CommandList;
		friend class DescriptorHeap;
		friend class ::ImGuiImpl;
	public:
		GPUBuffer(Context* context, std::uint64_t size);
		GPUBuffer(Context* context, std::uint64_t size, enums::BufferUsageFlag usage);
		GPUBuffer(Context* context, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage);
		virtual ~GPUBuffer();

		virtual void Map();
		virtual void Unmap();
		void Update(void* data, std::uint64_t size, std::uint64_t offset = 0);

	protected:
		void CreateBufferAndMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
				VkBuffer& buffer, VkDeviceMemory& memory);
		void Map_Internal(VkDeviceMemory memory);
		void Unmap_Internal(VkDeviceMemory memory);

		Context* m_context;

		std::uint64_t m_size;
		bool m_mapped;
		void* m_mapped_data;
		VkBuffer m_buffer;
		VkDeviceMemory m_buffer_memory;
	};

	class StagingBuffer : public GPUBuffer
	{
		friend class CommandList;
	public:
		StagingBuffer(Context* context, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage);
		~StagingBuffer() final;

		void Map() final;
		void Unmap() final;
		void FreeStagingResources();

	private:
		VkBuffer m_staging_buffer;
		VkDeviceMemory m_staging_buffer_memory;
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

		Texture(Context* context, Desc desc, bool uav = false);
		virtual ~Texture();

	protected:
		void CreateImageAndMemory(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		                          VkImage& image, VkDeviceMemory memory);
	private:
		Context* m_hidden_context;

	protected:
		Desc m_desc;
		VkImage m_texture;
		VkDeviceMemory m_texture_memory;
	};

	class StagingTexture : public GPUBuffer, public Texture
	{
		friend class DescriptorHeap;
		friend class CommandList;
	public:

		StagingTexture(Context* context, Desc desc);
		StagingTexture(Context* context, Desc desc, void* pixels);
		~StagingTexture() final = default;

		void FreeStagingResources();

	private:
	};

} /* gfx */
