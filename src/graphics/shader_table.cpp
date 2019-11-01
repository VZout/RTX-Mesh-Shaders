/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "shader_table.hpp"

#include "context.hpp"
#include "gpu_buffers.hpp"

template<typename T, typename A>
constexpr inline T SizeAlignTwoPower(T size, A alignment)
{
	return (size + (alignment - 1U)) & ~(alignment - 1U);
}

gfx::ShaderTable::ShaderTable(Context* context, std::uint64_t num_shader_records, std::uint64_t shader_record_size)
	: m_shader_record_size(shader_record_size), m_context(context)
{
	auto rt_properties = context->GetRayTracingDeviceProperties();

	m_shader_record_size = SizeAlignTwoPower(shader_record_size, rt_properties.shaderGroupBaseAlignment);
	m_shader_records.reserve(num_shader_records);

	CreateBuffer();

	auto data = (std::uint8_t*) m_buffer->m_mapped_data;

	// TODO: Replace nullptr and the indices with a enumerator.
	data += CopyShaderIdentifier(data, nullptr, 0);
	data += CopyShaderIdentifier(data, nullptr, 1);
	data += CopyShaderIdentifier(data, nullptr, 2);
}

std::uint64_t gfx::ShaderTable::CopyShaderIdentifier(uint8_t* data, const uint8_t* shader_handle_storage, uint32_t group_index)
{
	auto rt_properties = m_context->GetRayTracingDeviceProperties();

	const uint32_t shader_group_handle_size = rt_properties.shaderGroupHandleSize;
	memcpy(data, shader_handle_storage + group_index * shader_group_handle_size, shader_group_handle_size);
	data += shader_group_handle_size;
	return shader_group_handle_size;
}

void gfx::ShaderTable::CreateBuffer()
{
	m_buffer = new GPUBuffer(m_context, std::nullopt, m_shader_record_size, gfx::enums::BufferUsageFlag::RAYTRACING);
	m_buffer->Map();
}
