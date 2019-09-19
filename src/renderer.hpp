/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>
#include <cstdint>

#include "resource_structs.hpp"

class Application;
class ModelData;
class TinyGLTFModelLoader;
class ModelPool;
class TexturePool;
class MaterialPool;

namespace sg
{

	class SceneGraph;

} /* sg */

namespace fg
{

	class FrameGraph;

} /* fg */

namespace gfx
{

	class Context;
	class CommandQueue;
	class RenderWindow;
	class Shader;
	class Viewport;
	class PipelineState;
	class RootSignature;
	class RenderTarget;
	class CommandList;
	class GPUBuffer;
	class StagingBuffer;
	class Fence;
	class DescriptorHeap;
	class VkModelPool;
	class StagingTexture;
	class VkTexturePool;
	class VkMaterialPool;

} /* gfx */

class Renderer
{
public:
	Renderer();
	~Renderer();

	void Init(Application* app);
	void Upload();
	void Render(sg::SceneGraph& sg, fg::FrameGraph& fg);
	void WaitForAllPreviousWork();
	void Resize(std::uint32_t width, std::uint32_t height);
	Application* GetApp();
	std::uint32_t GetFrameIdx();
	ModelPool* GetModelPool();
	TexturePool* GetTexturePool();
	MaterialPool* GetMaterialPool();

	gfx::CommandList* CreateDirectCommandList(std::uint32_t num_versions);
	gfx::CommandList* CreateCopyCommandList(std::uint32_t num_versions);
	gfx::CommandList* CreateComputeCommandList(std::uint32_t num_versions);
	void ResetCommandList(gfx::CommandList* cmd_list);
	void StartRenderTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target);
	void StopRenderTask(gfx::CommandList* cmd_list, std::pair<gfx::RenderTarget*, RenderTargetProperties> render_target);
	void CloseCommandList(gfx::CommandList* cmd_list);
	void DestroyCommandList(gfx::CommandList* cmd_list);

	gfx::RenderTarget* CreateRenderTarget(RenderTargetProperties const & properties);
	void ResizeRenderTarget(gfx::RenderTarget* render_target, std::uint32_t width, std::uint32_t height);
	void DestroyRenderTarget(gfx::RenderTarget* render_target);
	gfx::RenderWindow* GetRenderWindow();

	// TODO: These need to be destroyed
	gfx::Context* GetContext() { return m_context; }
	gfx::CommandQueue* GetDirectQueue() { return m_direct_queue; };
	gfx::DescriptorHeap* GetDescHeap() { return m_desc_heap; };
	gfx::RootSignature* GetRootSignature() { return m_root_signature; };
	gfx::PipelineState* GetPipeline() { return m_pipeline; };
	gfx::RootSignature* GetCompoRootSignature() { return m_compo_root_signature; };
	gfx::PipelineState* GetCompoPipeline() { return m_compo_pipeline; };

private:
	Application* m_application;
	gfx::Context* m_context;
	gfx::CommandQueue* m_direct_queue;
	gfx::RenderWindow* m_render_window;
	gfx::CommandList* m_direct_cmd_list;
	std::vector<gfx::Fence*> m_present_fences;

	// TODO Temporary
	gfx::Shader* m_vs;
	gfx::Shader* m_ps;
	gfx::Shader* m_compo_vs;
	gfx::Shader* m_compo_ps;
	gfx::Viewport* m_viewport;
	gfx::DescriptorHeap* m_desc_heap;
	gfx::PipelineState* m_pipeline;
	gfx::RootSignature* m_root_signature;
	gfx::PipelineState* m_compo_pipeline;
	gfx::RootSignature* m_compo_root_signature;
	gfx::VkModelPool* m_model_pool;
	gfx::VkTexturePool* m_texture_pool;
	gfx::VkMaterialPool* m_material_pool;
};