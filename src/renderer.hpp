/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define IMGUI

#include <vector>
#include <cstdint>
#include <chrono>

class Application;
class ImGuiImpl;
class ModelData;
class TinyGLTFModelLoader;

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

} /* gfx */

class Renderer
{
public:
	Renderer();
	~Renderer();

	void Init(Application* app);
	void Render();
	void WaitForAllPreviousWork();
	void Resize(std::uint32_t width, std::uint32_t height);

private:
	gfx::Context* m_context;
	gfx::CommandQueue* m_direct_queue;
	gfx::RenderWindow* m_render_window;
	gfx::CommandList* m_direct_cmd_list;
	std::vector<gfx::Fence*> m_present_fences;

	// TODO Temporary
	gfx::Shader* m_vs;
	gfx::Shader* m_ps;
	gfx::Viewport* m_viewport;
	gfx::DescriptorHeap* m_desc_heap;
	std::vector<gfx::GPUBuffer*> m_cbs;
	gfx::PipelineState* m_pipeline;
	gfx::RootSignature* m_root_signature;
	TinyGLTFModelLoader* m_model_loader;
	ModelData* m_model;
	gfx::VkModelPool* m_model_pool;

	bool m_temp = false;

#ifdef IMGUI
	ImGuiImpl* imgui_impl;
#endif

	std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};