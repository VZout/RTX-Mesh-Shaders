/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "gfx_enums.hpp"

class ImGuiImpl;

namespace gfx
{

	class Context;

	class Shader
	{
		friend class ::ImGuiImpl;
		friend class PipelineState;
	public:
		explicit Shader(Context* context);
		~Shader();

		void Load(std::string const & path);
		void Compile(enums::ShaderType type);
		void LoadAndCompile(std::string const & path, enums::ShaderType type);

	private:
		Context* m_context;

		std::string m_path;
		enums::ShaderType m_type;
		std::vector<char> m_data; // Cleared after `Shader::Compile`.
		VkShaderModule m_module;
		VkPipelineShaderStageCreateInfo m_shader_stage_create_info;

	};

} /* gfx */