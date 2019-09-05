/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "shader.hpp"
#include "context.hpp"

#include <fstream>

gfx::Shader::Shader(Context* context)
	: m_context(context), m_module(VK_NULL_HANDLE)
{

}

gfx::Shader::~Shader()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyShaderModule(logical_device, m_module, nullptr);
}

void gfx::Shader::Load(std::string const& path)
{
	m_path = path;

	// Open file
	std::ifstream file(path.c_str(), std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	// Determine file size
	size_t file_size = (size_t) file.tellg();
	m_data.resize(file_size);

	// Read file
	file.seekg(0);
	file.read(m_data.data(), file_size);

	// Close file
	file.close();
}

void gfx::Shader::Compile(gfx::ShaderType type)
{
	m_type = type;
	VkShaderStageFlagBits vulkan_type;
	switch(m_type)
	{
		case ShaderType::VERTEX: vulkan_type = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderType::PIXEL: vulkan_type = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case ShaderType::COMPUTE: vulkan_type = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		case ShaderType::GEOMETRY: vulkan_type = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case ShaderType::MESH: vulkan_type = VK_SHADER_STAGE_MESH_BIT_NV;
			break;
		default:
			throw std::runtime_error("Tried to compile a shader with a unknown shader type.");
	}

	auto logical_device = m_context->m_logical_device;

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = m_data.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(m_data.data());

	if (vkCreateShaderModule(logical_device, &create_info, nullptr, &m_module) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_module;
	fragShaderStageInfo.pName = "main";

	m_data.clear();
}

void gfx::Shader::LoadAndCompile(std::string const& path, gfx::ShaderType type)
{
	Load(path);
	Compile(type);
}
