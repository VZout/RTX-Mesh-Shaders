/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace gfx
{

	enum class ShaderType
	{
		VERTEX,
		PIXEL,
		COMPUTE,
		GEOMETRY,
		MESH
	};

	class Context;

	class Shader
	{
		friend class PipelineState;
	public:
		explicit Shader(Context* context);
		~Shader();

		void Load(std::string const & path);
		void Compile(ShaderType type);
		void LoadAndCompile(std::string const & path, ShaderType type);

	private:
		std::string m_path;
		ShaderType m_type;
		VkPipelineShaderStageCreateInfo m_shader_stage_create_info;
		std::vector<char> m_data; // Cleared after `Shader::Compile`.
		VkShaderModule m_module;

		Context* m_context;
	};

} /* gfx */