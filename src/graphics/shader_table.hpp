/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace gfx
{

	class Context;
	class PipelineState;
	class GPUBuffer;

	struct ShaderRecord
	{
		std::pair<void*, std::uint64_t> m_shader_identifier;
		std::pair<void*, std::uint64_t> m_local_root_args;
	};

	class ShaderTable
	{
		friend class CommandList;
	public:
		ShaderTable(Context* context, std::uint64_t num_shader_records);
		~ShaderTable();

		std::uint64_t CopyShaderIdentifier(uint8_t* data, const uint8_t* shader_handle_storage, std::uint32_t group_index);
		void AddShaderRecord(PipelineState* pipeline_state, std::uint32_t first_group, std::uint32_t num_groups);

	private:
		void CreateBuffer();

		std::uint64_t m_shader_record_size = 0u;
		std::uint64_t m_buffer_size = 0u;
		std::uint64_t m_offset = 0u;
		std::vector<ShaderRecord> m_shader_records;
		GPUBuffer* m_buffer;

		Context* m_context;
	};

} /* gfx */