/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <util/delegate.hpp>
#include <frame_graph/frame_graph.hpp>
#include <render_tasks/vulkan_tasks.hpp>

namespace fg_manager
{

	using frame_graph_setup_func_t = util::Delegate<fg::FrameGraph*(Renderer*, decltype(tasks::ImGuiTaskData::m_render_func))>;

	enum class FGType : std::int32_t
	{
		PBR_GENERIC,
		PBR_MESH_SHADING,
		COUNT
	};

	inline std::string GetFrameGraphName(FGType id)
	{
		switch (id)
		{
		case FGType::PBR_GENERIC: return "PBR Generic";
		case FGType::PBR_MESH_SHADING: return "PBR Mesh Shading";
		default:
			return "Unknown";
		}
	}

	static std::array<frame_graph_setup_func_t, static_cast<std::size_t>(FGType::COUNT)> frame_graph_setup_functions =
	{
		// PBR Generic
		[](Renderer* rs, decltype(tasks::ImGuiTaskData::m_render_func) imgui_func)
		{
			auto fg = new fg::FrameGraph(9);
			tasks::AddGenerateCubemapTask(*fg);
			tasks::AddGenerateIrradianceMapTask(*fg);
			tasks::AddGenerateEnvironmentMapTask(*fg);
			tasks::AddGenerateBRDFLutTask(*fg);
			tasks::AddDeferredMainTask(*fg);
			tasks::AddDeferredCompositionTask(*fg);
			tasks::AddPostProcessingTask<tasks::DeferredCompositionData>(*fg);
			tasks::AddCopyToBackBufferTask<tasks::PostProcessingData>(*fg);
			tasks::AddImGuiTask<tasks::PostProcessingData>(*fg, imgui_func);

			fg->Validate();
			fg->Setup(rs);
			return fg;
		},
		// PBR Mesh Shading
		[](Renderer* rs, decltype(tasks::ImGuiTaskData::m_render_func) imgui_func)
		{
			auto fg = new fg::FrameGraph(9);
			tasks::AddGenerateCubemapTask(*fg);
			tasks::AddGenerateIrradianceMapTask(*fg);
			tasks::AddGenerateEnvironmentMapTask(*fg);
			tasks::AddGenerateBRDFLutTask(*fg);
			tasks::AddDeferredMainMeshTask(*fg);
			tasks::AddDeferredCompositionTask(*fg);
			tasks::AddPostProcessingTask<tasks::DeferredCompositionData>(*fg);
			tasks::AddCopyToBackBufferTask<tasks::PostProcessingData>(*fg);
			tasks::AddImGuiTask<tasks::PostProcessingData>(*fg, imgui_func);

			fg->Validate();
			fg->Setup(rs);
			return fg;
		},
	};

	static fg::FrameGraph* CreateFrameGraph(FGType type, Renderer* rs, decltype(tasks::ImGuiTaskData::m_render_func) imgui_func)
	{
		return frame_graph_setup_functions[static_cast<std::int32_t>(type)](rs, imgui_func);
	}

} /* fg_manager */