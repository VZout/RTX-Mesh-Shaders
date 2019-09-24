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
#include "graphics/vk_constant_buffer_pool.hpp"
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
#include "graphics/gfx_enums.hpp"
#include "graphics/gpu_buffers.hpp"
#include "graphics/fence.hpp"
#include "graphics/descriptor_heap.hpp"
#include "shader_registry.hpp"
#include "root_signature_registry.hpp"
#include "pipeline_registry.hpp"

Renderer::Renderer() : m_application(nullptr), m_context(nullptr), m_direct_queue(nullptr), m_render_window(nullptr), m_direct_cmd_list(nullptr)
{
	TexturePool::RegisterLoader<STBImageLoader>();
	ModelPool::RegisterLoader<TinyGLTFModelLoader>();
}

Renderer::~Renderer()
{
	WaitForAllPreviousWork();

	DestroyRegistry<ShaderRegistry>();
	DestroyRegistry<RootSignatureRegistry>();
	DestroyRegistry<PipelineRegistry>();

	delete m_viewport;
	for (auto& fence : m_present_fences)
	{
		delete fence;
	}
	delete m_desc_heap;
	delete m_texture_pool;
	delete m_model_pool;
	delete m_material_pool;
	delete m_render_window;
	delete m_direct_cmd_list;
	delete m_direct_queue;
	delete m_context;
}

void Renderer::Init(Application* app)
{
	m_application = app;
	m_context = new gfx::Context(app);

	LOG("Initialized Vulkan");

	m_viewport = new gfx::Viewport(app->GetWidth(), app->GetHeight());

	PrepareShaderRegistry();
	PrepareRootSignatureRegistry();
	PreparePipelineRegistry();

	m_render_window = new gfx::RenderWindow(m_context);
	m_direct_queue = new gfx::CommandQueue(m_context, gfx::CommandQueueType::DIRECT);
	m_direct_cmd_list = new gfx::CommandList(m_direct_queue);

	m_present_fences.resize(gfx::settings::num_back_buffers);
	for (auto& fence : m_present_fences)
	{
		fence = new gfx::Fence(m_context);
	}

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
	m_direct_cmd_list->TransitionDepth(m_render_window, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// Upload data to the gpu
	m_model_pool->Stage(m_direct_cmd_list);
	m_texture_pool->Stage(m_direct_cmd_list);
	m_direct_cmd_list->Close();

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
	// TODO: REMOVE THIS. use dyn viewport instead.
	PipelineRegistry::SFind(pipelines::basic)->Recompile();
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


void Renderer::PrepareShaderRegistry()
{
	auto& registry = ShaderRegistry::Get();
	auto const & descs = registry.GetDescriptions();
	auto& objects = registry.GetObjects();

	for (auto const & desc : descs)
	{
		auto shader = new gfx::Shader(m_context);
		shader->LoadAndCompile(desc.m_path, desc.m_type);
		objects.push_back(shader);
	}
}

void Renderer::PrepareRootSignatureRegistry()
{
	auto& registry = RootSignatureRegistry::Get();
	auto const & descs = registry.GetDescriptions();
	auto& objects = registry.GetObjects();

	for (auto const & desc : descs)
	{
		gfx::RootSignature::Desc n_desc;
		n_desc.m_parameters = desc.m_parameters;

		auto rs = new gfx::RootSignature(m_context, n_desc);
		rs->Compile();
		objects.push_back(rs);
	}
}

void Renderer::PreparePipelineRegistry()
{
	auto& registry = PipelineRegistry::Get();
	auto& rs_registry = RootSignatureRegistry::Get();
	auto& s_registry = ShaderRegistry::Get();
	auto const & descs = registry.GetDescriptions();
	auto& objects = registry.GetObjects();

	for (auto const & desc : descs)
	{
		gfx::PipelineState::Desc n_desc;
		n_desc.m_type = desc.m_type;
		n_desc.m_counter_clockwise = desc.m_counter_clockwise;
		n_desc.m_depth_format = desc.m_depth_format;
		n_desc.m_rtv_formats = desc.m_rtv_formats;
		n_desc.m_topology = desc.m_topology;

		auto ps = new gfx::PipelineState(m_context, n_desc);
		ps->SetRootSignature(rs_registry.Find(desc.m_root_signature_handle));
		for (auto const & shader_handle: desc.m_shader_handles)
		{
			ps->AddShader(s_registry.Find(shader_handle));
		}
		if (desc.m_input_layout.has_value()) ps->SetInputLayout(desc.m_input_layout.value());
		ps->SetViewport(m_viewport);
		ps->Compile();
		objects.push_back(ps);
	}
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
	auto frame_idx = GetFrameIdx();

	cmd_list->Begin(frame_idx);
}

void Renderer::StartRenderTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target)
{
	auto desc = render_target.second;

	if (render_target.second.m_is_render_window)
	{
		cmd_list->BindRenderTargetVersioned(render_target.first, desc.m_clear, desc.m_clear_depth);
	}
	else
	{
		if (!desc.m_is_render_window && desc.m_state_execute.has_value())
		{
			cmd_list->TransitionRenderTarget(render_target.first, VK_IMAGE_LAYOUT_UNDEFINED, desc.m_state_execute.value());
		}
		cmd_list->BindRenderTarget(render_target.first, desc.m_clear, desc.m_clear_depth);
	}
}

void Renderer::StopRenderTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target)
{
	auto desc = render_target.second;

	cmd_list->UnbindRenderTarget();

	if (!desc.m_is_render_window && desc.m_state_finished.has_value())
	{
		auto old = desc.m_state_execute.has_value() ? desc.m_state_execute.value() : VK_IMAGE_LAYOUT_UNDEFINED;
		cmd_list->TransitionRenderTarget(render_target.first, old, desc.m_state_finished.value());
	}
}

void Renderer::StartComputeTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target)
{
	auto desc = render_target.second;

	if (!desc.m_is_render_window && desc.m_state_execute.has_value())
	{
		cmd_list->TransitionRenderTarget(render_target.first, VK_IMAGE_LAYOUT_UNDEFINED, desc.m_state_execute.value());
	}
}

void Renderer::StopComputeTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target)
{
	auto desc = render_target.second;

	if (!desc.m_is_render_window && desc.m_state_finished.has_value())
	{
		auto old = desc.m_state_execute.has_value() ? desc.m_state_execute.value() : VK_IMAGE_LAYOUT_UNDEFINED;
		cmd_list->TransitionRenderTarget(render_target.first, old, desc.m_state_finished.value());
	}
}

void Renderer::CloseCommandList(gfx::CommandList* cmd_list)
{
	cmd_list->Close();
}

void Renderer::DestroyCommandList(gfx::CommandList* cmd_list)
{
	delete cmd_list;
}

gfx::RenderTarget* Renderer::CreateRenderTarget(RenderTargetProperties const & properties, bool compute)
{
	if (properties.m_is_render_window)
	{
		return m_render_window;
	}
	else
	{
		gfx::RenderTarget::Desc desc = {};
		desc.m_rtv_formats = properties.m_rtv_formats;
		desc.m_depth_format = properties.m_dsv_format;
		desc.m_width = properties.m_width.has_value() ? properties.m_width.value() : m_application->GetWidth();
		desc.m_height = properties.m_height.has_value() ? properties.m_height.value() : m_application->GetHeight();
		desc.m_allow_uav = compute;
		desc.m_clear = properties.m_clear;
		desc.m_clear_depth = properties.m_clear_depth;
		auto new_rt = new gfx::RenderTarget(m_context, desc);
		return new_rt;
	}
}

void Renderer::ResizeRenderTarget(gfx::RenderTarget* render_target, std::uint32_t width, std::uint32_t height)
{
	render_target->Resize(width, height);
}

void Renderer::DestroyRenderTarget(gfx::RenderTarget* render_target)
{
	delete render_target;
}

ConstantBufferPool* Renderer::CreateConstantBufferPool(std::uint32_t binding)
{
	return new gfx::VkConstantBufferPool(m_context, binding);
}

gfx::RenderWindow* Renderer::GetRenderWindow()
{
	return m_render_window;
}
