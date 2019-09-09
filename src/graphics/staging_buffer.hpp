/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

#include "gfx_enums.hpp"

namespace gfx
{

	class Context;

	class StagingBuffer
	{
		friend class CommandList;
	public:
		StagingBuffer(Context* context, void* data, std::uint64_t size, std::uint64_t stride, enums::BufferUsageFlag usage);
		~StagingBuffer();

		void Map();
		void Unmap();
		void Update(void* data, std::uint64_t size);
		void FreeStagingResources();

	private:
		void CreateBufferAndMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
				VkBuffer& buffer, VkDeviceMemory& memory);

		std::uint64_t m_size;
		std::uint64_t m_stride;
		bool m_mapped;
		void* m_mapped_data;
		VkBuffer m_buffer;
		VkBuffer m_staging_buffer;
		VkDeviceMemory m_buffer_memory;
		VkDeviceMemory m_staging_buffer_memory;

		Context* m_context;
	};

} /* gfx */