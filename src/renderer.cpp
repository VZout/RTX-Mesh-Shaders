#include "renderer.hpp"

#include <iostream>

Renderer::~Renderer()
{
	delete m_direct_queue;
	delete m_context;
}

void Renderer::Init()
{
	m_context = new gfx::Context();

	std::cout << "Initialized Vulkan" << std::endl;

	auto supported_extensions = m_context->GetSupportedExtensions();

	std::cout << "Supported Extensions:" << std::endl;
	for (auto extension : supported_extensions)
	{
		std::cout << "\t- " << extension.extensionName << " (" << std::to_string(extension.specVersion) << ")" << std::endl;
	}

	m_direct_queue = new gfx::CommandQueue(m_context, gfx::CommandQueueType::DIRECT);
}

void Renderer::Render()
{

}