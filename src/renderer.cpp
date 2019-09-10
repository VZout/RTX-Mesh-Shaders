/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "renderer.hpp"

#include "application.hpp"
#include "vertex.hpp"
#include "buffer_definitions.hpp"
#include "graphics/context.hpp"
#include "graphics/command_queue.hpp"
#include "graphics/render_window.hpp"
#include "graphics/shader.hpp"
#include "graphics/pipeline_state.hpp"
#include "graphics/viewport.hpp"
#include "graphics/root_signature.hpp"
#include "graphics/command_list.hpp"
#include "graphics/gfx_settings.hpp"
#include "graphics/gfx_enums.hpp"
#include "graphics/gpu_buffers.hpp"
#include "graphics/fence.hpp"
#include "graphics/descriptor_heap.hpp"
#include <imgui.h>
#include "imgui/imgui_style.hpp"
#include "imgui/imgui_impl_glfw.hpp"
#include "imgui/imgui_impl_vulkan.hpp"

#include <iostream>

Renderer::Renderer() : m_context(nullptr), m_direct_queue(nullptr), m_render_window(nullptr), m_direct_cmd_list(nullptr)
{

}

Renderer::~Renderer()
{
	WaitForAllPreviousWork();

#ifdef IMGUI
	delete imgui_impl;
#endif

	ImGui_ImplGlfw_Shutdown();

	for (auto& cb : m_cbs)
	{
		delete cb;
	}
	delete m_viewport;
	for (auto& fence : m_present_fences)
	{
		delete fence;
	}
	delete m_root_signature;
	delete m_pipeline;
	delete m_vs;
	delete m_ps;
	delete m_render_window;
	delete m_direct_cmd_list;
	delete m_direct_queue;
	delete m_desc_heap;
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
	m_direct_cmd_list = new gfx::CommandList(m_direct_queue);

	m_present_fences.resize(gfx::settings::num_back_buffers);
	for (auto& fence : m_present_fences)
	{
		fence = new gfx::Fence(m_context);
	}

	m_vs = new gfx::Shader(m_context);
	m_ps = new gfx::Shader(m_context);
	m_vs->LoadAndCompile("shaders/triangle.vert.spv", gfx::ShaderType::VERTEX);
	m_ps->LoadAndCompile("shaders/triangle.frag.spv", gfx::ShaderType::PIXEL);

	m_viewport = new gfx::Viewport(app->GetWidth(), app->GetHeight());

	gfx::RootSignature::Desc root_signature_desc;
	root_signature_desc.m_parameters.resize(1);
	root_signature_desc.m_parameters[0].binding = 0;
	root_signature_desc.m_parameters[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	root_signature_desc.m_parameters[0].descriptorCount = 1;
	root_signature_desc.m_parameters[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	root_signature_desc.m_parameters[0].pImmutableSamplers = nullptr;
	m_root_signature = new gfx::RootSignature(m_context, root_signature_desc);
	m_root_signature->Compile();

	m_pipeline = new gfx::PipelineState(m_context);
	m_pipeline->SetViewport(m_viewport);
	m_pipeline->AddShader(m_vs);
	m_pipeline->AddShader(m_ps);
	m_pipeline->SetRenderTarget(m_render_window);
	m_pipeline->SetRootSignature(m_root_signature);
	m_pipeline->Compile();

	gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
	descriptor_heap_desc.m_versions = gfx::settings::num_back_buffers;
	descriptor_heap_desc.m_num_descriptors = 1;
	m_desc_heap = new gfx::DescriptorHeap(m_context, m_root_signature, descriptor_heap_desc);
	m_cbs.resize(gfx::settings::num_back_buffers);
	for (auto i = 0; i < gfx::settings::num_back_buffers; i++)
	{
		m_cbs[i] = new gfx::GPUBuffer(m_context, sizeof(cb::Basic), gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
		m_cbs[i]->Map();
		m_desc_heap->CreateSRVFromCB(m_cbs[i], 0, i);
	}

	m_start = std::chrono::high_resolution_clock::now();

#ifdef IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	io.ConfigDockingWithShift = true;

	ImGui_ImplGlfw_InitForVulkan(app->GetWindow(), true);

	// Setup Dear ImGui style
	ImGui::StyleColorsCherry();

	imgui_impl = new ImGuiImpl();
	imgui_impl->InitImGuiResources(m_context, m_render_window, m_direct_queue);
	std::cout << "Finished Initializing ImGui" << std::endl;
#endif

	std::cout << "Finished Initializing Renderer" << std::endl;
}

void Renderer::Render()
{
	auto frame_idx = m_render_window->GetFrameIdx();

	m_present_fences[frame_idx]->Wait();
	m_render_window->AquireBackBuffer(m_present_fences[frame_idx]);

	auto diff = std::chrono::high_resolution_clock::now() - m_start;
	float t = diff.count();
	cb::Basic basic_cb_data { t };
	m_cbs[frame_idx]->Update(&basic_cb_data, sizeof(cb::Basic));

	m_direct_cmd_list->Begin(frame_idx);
	m_direct_cmd_list->BindRenderTargetVersioned(m_render_window, frame_idx);

	m_direct_cmd_list->BindPipelineState(m_pipeline, frame_idx);
	m_direct_cmd_list->BindDescriptorTable(m_root_signature, m_desc_heap, 0, frame_idx);
	m_direct_cmd_list->Draw(frame_idx, 4, 1);

#ifdef IMGUI
	// imgui itself code
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("About"))
		{
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::Begin("Whatsup");
	ImGui::Text("Hey this is my framerate: %.0f", ImGui::GetIO().Framerate);
	ImGui::ToggleButton("Toggle", &m_temp);
	ImGui::End();

	ImGui::Begin("Vulkan Details");

	ImGui::End();

	// Render to generate draw buffers
	ImGui::Render();

	imgui_impl->UpdateBuffers();
	imgui_impl->Draw(m_direct_cmd_list, frame_idx);
#endif

	m_direct_cmd_list->Close(frame_idx);

	m_direct_queue->Execute({ m_direct_cmd_list }, m_present_fences[frame_idx], frame_idx);

	m_render_window->Present(m_direct_queue, m_present_fences[frame_idx]);
}

void Renderer::WaitForAllPreviousWork()
{
	for (auto& fence : m_present_fences)
	{
		fence->Wait();
	}
}

void Renderer::Resize(std::uint32_t width, std::uint32_t height)
{
	//WaitForAllPreviousWork();
	m_context->WaitForDevice();

	m_viewport->Resize(width, height);

	m_render_window->Resize(width, height);
	m_pipeline->SetRenderTarget(m_render_window);
	m_pipeline->Recompile();
}
