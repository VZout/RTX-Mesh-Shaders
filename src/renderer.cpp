/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "renderer.hpp"

#include "util/log.hpp"
#include "application.hpp"
#include "texture_pool.hpp"
#include "stb_image_loader.hpp"
#include "tinygltf_model_loader.hpp"
#include "vertex.hpp"
#include "frame_graph/frame_graph.hpp"
#include "scene_graph/scene_graph.hpp"
#include "buffer_definitions.hpp"
#include "graphics/vk_material_pool.hpp"
#include "graphics/context.hpp"
#include "graphics/vk_model_pool.hpp"
#include "graphics/vk_texture_pool.hpp"
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

Renderer::Renderer() : m_application(nullptr), m_context(nullptr), m_direct_queue(nullptr), m_render_window(nullptr), m_direct_cmd_list(nullptr)
{
	TexturePool::RegisterLoader<STBImageLoader>();
	ModelPool::RegisterLoader<TinyGLTFModelLoader>();
}

Renderer::~Renderer()
{
	WaitForAllPreviousWork();

	delete m_viewport;
	for (auto& fence : m_present_fences)
	{
		delete fence;
	}
	delete m_texture_pool;
	delete m_model_pool;
	delete m_material_pool;
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
	m_application = app;
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
	descriptor_heap_desc.m_num_descriptors = 100;
	m_desc_heap = new gfx::DescriptorHeap(m_context, descriptor_heap_desc);

	m_model_pool = new gfx::VkModelPool(m_context);
	m_texture_pool = new gfx::VkTexturePool(m_context);
	m_material_pool = new gfx::VkMaterialPool(m_context);

	LOG("Finished Initializing Renderer");
}

void Renderer::Upload()
{
	m_direct_cmd_list->Begin(0);

	// make sure the data depth buffer is ready for present
	m_direct_cmd_list->TransitionDepth(m_render_window, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0);

	// Upload data to the gpu
	m_model_pool->Stage(m_direct_cmd_list, 0);
	m_texture_pool->Stage(m_direct_cmd_list, 0);
	m_direct_cmd_list->Close(0);

	m_direct_queue->Execute({ m_direct_cmd_list }, nullptr, 0);
	m_direct_queue->Wait();
	m_model_pool->PostStage();
	m_texture_pool->PostStage();

	LOG("Finished Uploading Resources");
}

void Renderer::Render(sg::SceneGraph& sg, fg::FrameGraph& fg)
{
	auto frame_idx = m_render_window->GetFrameIdx();

	m_present_fences[frame_idx]->Wait();
	m_render_window->AquireBackBuffer(m_present_fences[frame_idx]);

	fg.Execute(sg);

	auto fg_cmd_lists = fg.GetAllCommandLists<gfx::CommandList>();
	m_direct_queue->Execute(fg_cmd_lists, m_present_fences[frame_idx], frame_idx);

	m_render_window->Present(m_direct_queue, m_present_fences[frame_idx]);
}

void Renderer::WaitForAllPreviousWork()
{
	m_context->WaitForDevice();
}

void Renderer::Resize(std::uint32_t width, std::uint32_t height)
{
	WaitForAllPreviousWork();

	m_viewport->Resize(width, height);

	m_render_window->Resize(width, height);
	m_pipeline->SetRenderTarget(m_render_window);
	m_pipeline->Recompile();
}

Application* Renderer::GetApp()
{
	return m_application;
}

std::uint32_t Renderer::GetFrameIdx()
{
	return m_render_window->GetFrameIdx();
}

ModelPool* Renderer::GetModelPool()
{
	return m_model_pool;
}

TexturePool* Renderer::GetTexturePool()
{
	return m_texture_pool;
}

MaterialPool* Renderer::GetMaterialPool()
{
	return m_material_pool;
}

gfx::CommandList* Renderer::CreateDirectCommandList(std::uint32_t num_versions)
{
	return new gfx::CommandList(m_direct_queue);
}

gfx::CommandList* Renderer::CreateCopyCommandList(std::uint32_t num_versions)
{
	return new gfx::CommandList(m_direct_queue);
}

gfx::CommandList* Renderer::CreateComputeCommandList(std::uint32_t num_versions)
{
	return new gfx::CommandList(m_direct_queue);
}

void Renderer::ResetCommandList(gfx::CommandList* cmd_list)
{
	// Nothing to be done for a vulkan command list
}

void Renderer::StartRenderTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target)
{
	auto frame_idx = m_render_window->GetFrameIdx();
	auto desc = render_target.second;

	cmd_list->Begin(frame_idx);
	if (render_target.second.m_is_render_window)
	{
		cmd_list->BindRenderTargetVersioned(render_target.first, frame_idx, desc.m_clear, desc.m_clear_depth);
	}
	else
	{
		LOGW("Coming soon");
	}
}

void Renderer::StopRenderTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target)
{
	// COMING SOON
}

void Renderer::CloseCommandList(gfx::CommandList* cmd_list)
{
	auto frame_idx = m_render_window->GetFrameIdx();

	cmd_list->Close(frame_idx);
}

void Renderer::DestroyCommandList(gfx::CommandList* cmd_list)
{
	delete cmd_list;
}

gfx::RenderTarget* Renderer::CreateRenderTarget(RenderTargetProperties const & properties)
{
	if (properties.m_is_render_window)
	{
		return m_render_window;
	}
	else
	{
		LOGW("Coming soon!");
	}
}

void Renderer::DestroyRenderTarget(gfx::RenderTarget* render_target)
{
	delete render_target;
}