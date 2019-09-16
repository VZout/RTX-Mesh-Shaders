/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <chrono>
#include <gtc/matrix_transform.hpp>

#include "../application.hpp"
#include "../buffer_definitions.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../renderer.hpp"
#include "../model_pool.hpp"
#include "../texture_pool.hpp"
#include "../vertex.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../tinygltf_model_loader.hpp"
#include "../graphics/vk_model_pool.hpp"
#include "../graphics/descriptor_heap.hpp"

namespace tasks
{

	struct DeferredMainData
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
		std::vector<gfx::GPUBuffer*> m_cbs;
		std::vector<std::vector<std::uint32_t>> m_cb_sets;
		std::vector<std::vector<std::uint32_t>> m_material_sets;
		ModelData* m_model;
	};

	namespace internal
	{

		inline void SetupDeferredMainTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& data = fg.GetData<DeferredMainData>(handle);
			auto context = rs.GetContext();
			auto desc_heap = rs.GetDescHeap();
			auto root_sig = rs.GetRootSignature();
			auto model_pool = rs.GetModelPool();
			auto texture_pool = rs.GetTexturePool();
			auto model_loader = rs.GetModelLoader();

			data.m_model = model_loader->Load("scene.gltf");
			model_pool->Load<Vertex>(data.m_model);

			data.m_cb_sets.resize(gfx::settings::num_back_buffers);
			data.m_material_sets.resize(gfx::settings::num_back_buffers);
			data.m_start = std::chrono::high_resolution_clock::now();

			// Descriptors uniform
			data.m_cbs.resize(gfx::settings::num_back_buffers);
			for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
			{
				data.m_cbs[i] = new gfx::GPUBuffer(context, sizeof(cb::Basic), gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
				data.m_cbs[i]->Map();

				data.m_cb_sets[i].push_back(desc_heap->CreateSRVFromCB(data.m_cbs[i], root_sig, 0, i));
			}

			// Textures
			std::vector<std::uint32_t> texture_handles;
			for (std::size_t i = 0; i < data.m_model->m_materials.size(); i++)
			{
				auto albedo_data = data.m_model->m_materials[i].m_albedo_texture;
				auto normal_data = data.m_model->m_materials[i].m_normal_map_texture;

				auto albedo_id = texture_pool->Load(albedo_data);
				auto normal_id = texture_pool->Load(normal_data);

				auto textures = texture_pool->GetTextures({ albedo_id, normal_id });
				for (auto frame_idx = 0u; frame_idx < gfx::settings::num_back_buffers; frame_idx++)
				{
					data.m_material_sets[frame_idx].push_back(desc_heap->CreateSRVSetFromTexture(textures, root_sig, 1, frame_idx));
				}
			}
		}

		inline void ExecuteDeferredMainTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredMainData>(handle);
			auto render_window = fg.GetRenderTarget<gfx::RenderWindow>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto frame_idx = rs.GetFrameIdx();
			auto model_pool = static_cast<gfx::VkModelPool*>(rs.GetModelPool());
			auto texture_pool = rs.GetTexturePool();
			auto root_sig = rs.GetRootSignature();
			auto pipeline = rs.GetPipeline();
			auto desc_heap = rs.GetDescHeap();

			auto diff = std::chrono::high_resolution_clock::now() - data.m_start;
			float t = diff.count();
			cb::Basic basic_cb_data;
			basic_cb_data.m_time = t;
			float size = 0.01;
			basic_cb_data.m_model = glm::mat4(1);
			basic_cb_data.m_model = glm::scale(basic_cb_data.m_model, glm::vec3(size));
			basic_cb_data.m_model = glm::rotate(basic_cb_data.m_model, glm::radians(t * 0.0000001f), glm::vec3(0, 1, 0));
			basic_cb_data.m_model = glm::rotate(basic_cb_data.m_model, glm::radians(-90.f), glm::vec3(1, 0, 0));
			basic_cb_data.m_view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			basic_cb_data.m_proj = glm::perspective(glm::radians(45.0f), (float) render_window->GetWidth() / (float) render_window->GetHeight(), 0.01f, 1000.0f);
			basic_cb_data.m_proj[1][1] *= -1;
			data.m_cbs[frame_idx]->Update(&basic_cb_data, sizeof(cb::Basic));

			cmd_list->BindPipelineState(pipeline, frame_idx);
			for (std::size_t i = 0; i < model_pool->m_vertex_buffers.size(); i++)
			{
				cmd_list->BindDescriptorHeap(root_sig, desc_heap, { data.m_cb_sets[frame_idx][0], data.m_material_sets[frame_idx][i] } , frame_idx);
				cmd_list->BindVertexBuffer(model_pool->m_vertex_buffers[i], frame_idx);
				cmd_list->BindIndexBuffer(model_pool->m_index_buffers[i], frame_idx);
				cmd_list->DrawIndexed(frame_idx, data.m_model->m_meshes[i].m_indices.size(), 1);
			}
		}

		inline void DestroyDeferredMainTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DeferredMainData>(handle);

			if (resize)
			{

			}
			else
			{
				for (auto& cb : data.m_cbs)
				{
					delete cb;
				}
			}
		}

	} /* internal */

	inline void AddDeferredMainTask(fg::FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			.m_is_render_window = true,
			.m_width = std::nullopt,
			.m_height = std::nullopt,
			.m_clear = true,
			.m_clear_depth = true,
		};

		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupDeferredMainTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			internal::ExecuteDeferredMainTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyDeferredMainTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = fg::RenderTaskType::DIRECT;
		desc.m_allow_multithreading = false;

		fg.AddTask<DeferredMainData>(desc, L"Deferred Rasterization Task");
	}

} /* tasks */
