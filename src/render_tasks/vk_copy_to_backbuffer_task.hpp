/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "../application.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../renderer.hpp"

namespace tasks
{

	struct CopyToBackBufferData
	{
	};

	namespace internal
	{

		template<typename T, std::uint32_t IDX>
		inline void ExecuteCopyToBackBufferTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph&, fg::RenderTaskHandle handle)
		{
			auto rt_to_copy = fg.GetPredecessorRenderTarget<T>();
			auto cmd_list = fg.GetCommandList(handle);
			auto render_window = rs.GetRenderWindow();
			auto frame_idx = rs.GetFrameIdx();

			cmd_list->TransitionRenderTarget(render_window, frame_idx, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			cmd_list->CopyRenderTargetToRenderWindow(rt_to_copy, IDX, render_window);

			cmd_list->TransitionRenderTarget(render_window, frame_idx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}

	} /* internal */

	template<typename T, std::uint32_t IDX = 0>
	inline void AddCopyToBackBufferTask(fg::FrameGraph& fg)
	{
		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer&, fg::FrameGraph&, ::fg::RenderTaskHandle, bool)
		{
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteCopyToBackBufferTask<T, IDX>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph&, ::fg::RenderTaskHandle, bool)
		{
		};

		desc.m_properties = std::nullopt;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		fg.AddTask<CopyToBackBufferData>(desc, "Copy to back buffer Task");
	}

} /* tasks */
