/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "renderer.hpp"

#include "util/log.hpp"
#include "application.hpp"
#include "tinygltf_model_loader.hpp"
#include "vertex.hpp"
#include "buffer_definitions.hpp"
#include "graphics/context.hpp"
#include "graphics/vk_model_pool.hpp"
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
#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

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
	for (auto& texture : m_textures)
	{
		delete texture;
	}
	delete m_model_loader;
	delete m_model_pool;
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

	LOG("Initialized Vulkan");

	auto supported_extensions = m_context->GetSupportedExtensions();
	auto supported_device_extensions = m_context->GetSupportedDeviceExtensions();

	auto print_extensions_func = [](auto extensions)
	{
		for (auto extension : extensions)
		{
			LOG("\t- {} ({})", extension.extensionName, std::to_string(extension.specVersion));
		}
	};

	LOG("Supported Instance Extensions:");
	print_extensions_func(supported_extensions);

	LOG("Supported Device Extensions:");
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
	m_vs->LoadAndCompile("shaders/basic.vert.spv", gfx::ShaderType::VERTEX);
	m_ps->LoadAndCompile("shaders/basic.frag.spv", gfx::ShaderType::PIXEL);

	m_viewport = new gfx::Viewport(app->GetWidth(), app->GetHeight());

	gfx::RootSignature::Desc root_signature_desc;
	root_signature_desc.m_parameters.resize(2);
	root_signature_desc.m_parameters[0].binding = 0; // root parameter 0
	root_signature_desc.m_parameters[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	root_signature_desc.m_parameters[0].descriptorCount = 1;
	root_signature_desc.m_parameters[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	root_signature_desc.m_parameters[0].pImmutableSamplers = nullptr;
	root_signature_desc.m_parameters[1].binding = 1; // root parameter 1
	root_signature_desc.m_parameters[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	root_signature_desc.m_parameters[1].descriptorCount = 2;
	root_signature_desc.m_parameters[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	root_signature_desc.m_parameters[1].pImmutableSamplers = nullptr;
	m_root_signature = new gfx::RootSignature(m_context, root_signature_desc);
	m_root_signature->Compile();

	m_pipeline = new gfx::PipelineState(m_context);
	m_pipeline->SetViewport(m_viewport);
	m_pipeline->AddShader(m_vs);
	m_pipeline->SetInputLayout(Vertex::GetInputLayout());
	m_pipeline->AddShader(m_ps);
	m_pipeline->SetRenderTarget(m_render_window);
	m_pipeline->SetRootSignature(m_root_signature);
	m_pipeline->Compile();

	gfx::DescriptorHeap::Desc descriptor_heap_desc = {};
	descriptor_heap_desc.m_versions = gfx::settings::num_back_buffers;
	descriptor_heap_desc.m_num_descriptors = 4;
	m_desc_heap = new gfx::DescriptorHeap(m_context, m_root_signature, descriptor_heap_desc);
	m_cbs.resize(gfx::settings::num_back_buffers);
	for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
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
	LOG("Finished Initializing ImGui");
#endif

	m_model_loader = new TinyGLTFModelLoader();
	m_model = m_model_loader->Load("scene.gltf");
	m_model_pool = new gfx::VkModelPool(m_context);
	m_model_pool->Load<Vertex>(m_model);

	m_textures.resize(m_model->m_materials.size());
	for (std::size_t i = 0; i < m_textures.size(); i++)
	{
		auto texture_data = m_model->m_materials[i].m_albedo_texture;
		gfx::StagingTexture::Desc texture_desc;
		texture_desc.m_width = texture_data.m_width;
		texture_desc.m_height = texture_data.m_height;
		texture_desc.m_format = VK_FORMAT_R8G8B8A8_UNORM;
		m_textures[i] = new gfx::StagingTexture(m_context, texture_desc, texture_data.m_pixels.data());
	}

	m_direct_cmd_list->Begin(0);

	// make sure the data depth buffer is ready for present
	m_direct_cmd_list->TransitionDepth(m_render_window, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0);

	// Make the staging texture ready for write. than stage. than transition to read optimal
	for (auto& texture : m_textures)
	{
		m_direct_cmd_list->TransitionTexture(texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);
		m_direct_cmd_list->StageTexture(texture, 0);
		m_direct_cmd_list->TransitionTexture(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);
	}

	// Upload data to the gpu
	m_model_pool->Stage(m_direct_cmd_list, 0);
	m_direct_cmd_list->Close(0);

	m_direct_queue->Execute({ m_direct_cmd_list }, nullptr, 0);
	m_direct_queue->Wait();
	m_model_pool->PostStage();
	for (auto& texture : m_textures)
	{
		texture->FreeStagingResources();
	}

	// Create texture shader resource views
	for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
	{
		m_desc_heap->CreateSRVSetFromTexture({ m_textures[0], m_textures[1] }, 1, i); // + 1 becasue 0 is uniform
	}

	LOG("Finished Uploading Resources");

	LOG("Finished Initializing Renderer");
}

void Renderer::Render()
{
	auto frame_idx = m_render_window->GetFrameIdx();

	m_present_fences[frame_idx]->Wait();
	m_render_window->AquireBackBuffer(m_present_fences[frame_idx]);

	auto diff = std::chrono::high_resolution_clock::now() - m_start;
	float t = diff.count();
	cb::Basic basic_cb_data;
	basic_cb_data.m_time = t;
	float size = 0.01;
	basic_cb_data.m_model = glm::mat4(1);
	basic_cb_data.m_model = glm::scale(basic_cb_data.m_model, glm::vec3(size));
	basic_cb_data.m_model = glm::rotate(basic_cb_data.m_model, glm::radians(t * 0.0000001f), glm::vec3(0, 1, 0));
	basic_cb_data.m_model = glm::rotate(basic_cb_data.m_model, glm::radians(-90.f), glm::vec3(1, 0, 0));
	basic_cb_data.m_view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	basic_cb_data.m_proj = glm::perspective(glm::radians(45.0f), (float) m_render_window->GetWidth() / (float) m_render_window->GetHeight(), 0.01f, 1000.0f);
	basic_cb_data.m_proj[1][1] *= -1;
	m_cbs[frame_idx]->Update(&basic_cb_data, sizeof(cb::Basic));

	m_direct_cmd_list->Begin(frame_idx);
	m_direct_cmd_list->BindRenderTargetVersioned(m_render_window, frame_idx);
	m_direct_cmd_list->BindPipelineState(m_pipeline, frame_idx);
	m_direct_cmd_list->BindDescriptorHeap(m_root_signature, m_desc_heap, frame_idx);
	for (std::size_t i = 0; i < m_model_pool->m_vertex_buffers.size(); i++)
	{
		m_direct_cmd_list->BindVertexBuffer(m_model_pool->m_vertex_buffers[i], frame_idx);
		m_direct_cmd_list->BindIndexBuffer(m_model_pool->m_index_buffers[i], frame_idx);
		m_direct_cmd_list->DrawIndexed(frame_idx, m_model->m_meshes[i].m_indices.size(), 1);
	}

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
