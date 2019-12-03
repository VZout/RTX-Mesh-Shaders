/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "shader_table.hpp"

#include "context.hpp"
#include "pipeline_state.hpp"
#include "gpu_buffers.hpp"

gfx::ShaderTable::ShaderTable(Context* context, std::uint64_t num_shader_records)
	: m_context(context)
{
	auto rt_properties = context->GetRayTracingDeviceProperties();
	m_shader_record_size = rt_properties.shaderGroupHandleSize;

	m_buffer_size = m_shader_record_size * num_shader_records;
	m_shader_records.reserve(num_shader_records);

	CreateBuffer();
}

gfx::ShaderTable::~ShaderTable()
{
	delete m_buffer;
}

std::uint64_t gfx::ShaderTable::CopyShaderIdentifier(uint8_t* data, const uint8_t* shader_handle_storage, std::uint32_t group_index)
{
	memcpy(data, shader_handle_storage + group_index * m_shader_record_size, m_shader_record_size);
	return m_shader_record_size;
}

void gfx::ShaderTable::AddShaderRecord(PipelineState* pipeline_state, std::uint32_t first_group, std::uint32_t num_groups)
{
	auto logical_device = m_context->m_logical_device;

	auto shader_record_storage = new uint8_t[m_shader_record_size * num_groups];

	if (Context::vkGetRayTracingShaderGroupHandlesNV(logical_device, pipeline_state->m_pipeline,
		first_group, num_groups, m_shader_record_size * num_groups, shader_record_storage) != VK_SUCCESS)
	{
		LOGE("Failed to get ray tracing shader group handles.");
	}

	auto data = static_cast<std::uint8_t*>(m_buffer->m_mapped_data);
	for (auto i = 0; i < num_groups; i++)
	{
		m_offset += CopyShaderIdentifier(data + m_offset, shader_record_storage, first_group + i);
	}

	delete[] shader_record_storage;
}

void gfx::ShaderTable::CreateBuffer()
{
	m_buffer = new GPUBuffer(m_context, std::nullopt, m_buffer_size, gfx::enums::BufferUsageFlag::RAYTRACING);
	m_buffer->Map();
}
