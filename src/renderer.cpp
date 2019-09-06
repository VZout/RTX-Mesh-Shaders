/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "renderer.hpp"

#include "application.hpp"
#include "graphics/context.hpp"
#include "graphics/command_queue.hpp"
#include "graphics/render_window.hpp"
#include "graphics/shader.hpp"
#include "graphics/pipeline_state.hpp"
#include "graphics/viewport.hpp"
#include "graphics/root_signature.hpp"
#include "graphics/render_pass.hpp"

#include <iostream>

Renderer::~Renderer()
{
	delete m_viewport;
	delete m_root_signature;
	delete m_render_pass;
	delete m_pipeline;
	delete m_vs;
	delete m_ps;
	delete m_render_window;
	delete m_direct_queue;
	delete m_context;
}

void Renderer::Init(Application* app)
{
	m_context = new gfx::Context(app);

	std::cout << "Initialized Vulkan" << std::endl;

	auto supported_extensions = m_context->GetSupportedExtensions();
	auto supported_device_extensions = m_context->GetSupportedDeviceExtensions();

	auto print_extensions_func = [](auto extensions)
	{
		for (auto extension : extensions)
		{
			std::cout << "\t- " << extension.extensionName << " (" << std::to_string(extension.specVersion) << ")" << std::endl;
		}
	};

	std::cout << "Supported Instance Extensions:" << std::endl;
	print_extensions_func(supported_extensions);

	std::cout << "Supported Device Extensions:" << std::endl;
	print_extensions_func(supported_device_extensions);

	m_render_window = new gfx::RenderWindow(m_context);
	m_direct_queue = new gfx::CommandQueue(m_context, gfx::CommandQueueType::DIRECT);

	m_vs = new gfx::Shader(m_context);
	m_ps = new gfx::Shader(m_context);
	m_vs->LoadAndCompile("vert.spv", gfx::ShaderType::VERTEX);
	m_ps->LoadAndCompile("pix.spv", gfx::ShaderType::PIXEL);

	m_viewport = new gfx::Viewport(app->GetWidth(), app->GetHeight());

	m_root_signature = new gfx::RootSignature(m_context);
	m_root_signature->Compile();

	m_render_pass = new gfx::RenderPass(m_context, VK_FORMAT_B8G8R8_UNORM);

	m_pipeline = new gfx::PipelineState(m_context);
	m_pipeline->SetViewport(m_viewport);
	m_pipeline->AddShader(m_vs);
	m_pipeline->AddShader(m_ps);
	m_pipeline->SetRenderPass(m_render_pass);
	m_pipeline->SetRootSignature(m_root_signature);
	m_pipeline->Compile();
}

void Renderer::Render()
{

}