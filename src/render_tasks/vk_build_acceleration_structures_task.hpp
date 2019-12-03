/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "../application.hpp"
#include "../buffer_definitions.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../renderer.hpp"
#include "../model_pool.hpp"
#include "../texture_pool.hpp"
#include "../vertex.hpp"
#include "../engine_registry.hpp"
#include "../imgui/imgui_impl_vulkan.hpp"
#include "../graphics/vk_model_pool.hpp"
#include "../graphics/descriptor_heap.hpp"
#include "../graphics/vk_material_pool.hpp"
#include "../graphics/gfx_enums.hpp"
#include "../graphics/acceleration_structure.hpp"

namespace tasks
{
	struct RaytracingOffset
	{
		std::uint32_t m_vertex_offset;
		std::uint32_t m_index_offset;
	};

	struct RaytracingMaterial
	{
		std::uint32_t m_albedo_texture;
		std::uint32_t m_normal_texture;
		std::uint32_t m_roughness_texture;
	};

	struct BuildASData
	{
		gfx::AccelerationStructure* m_tlas;
		std::vector<gfx::AccelerationStructure*> m_blasses;
		gfx::GPUBuffer* m_scratch_buffer;

		std::vector<RaytracingOffset> m_offsets;
		std::vector<RaytracingMaterial> m_materials;
		gfx::GPUBuffer* m_offsets_buffer;
		gfx::GPUBuffer* m_materials_buffer;

		bool m_should_build = true;
	};

	namespace internal
	{

		inline void SetupBuildASTask(Renderer& rs, fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<BuildASData>(handle);

			const auto scratch_size = 65536*20;

			data.m_offsets.resize(gfx::settings::max_num_rtx_materials);
			data.m_materials.resize(gfx::settings::max_num_rtx_materials);

			data.m_offsets_buffer = new gfx::GPUBuffer(rs.GetContext(), std::nullopt, gfx::settings::max_num_rtx_materials * sizeof(RaytracingOffset), gfx::enums::BufferUsageFlag::RAYTRACING_STORAGE);
			data.m_materials_buffer = new gfx::GPUBuffer(rs.GetContext(), std::nullopt, gfx::settings::max_num_rtx_materials * sizeof(RaytracingMaterial), gfx::enums::BufferUsageFlag::RAYTRACING_STORAGE);
			//data.m_scratch_buffer = new gfx::GPUBuffer(rs.GetContext(), std::nullopt, scratch_size, gfx::enums::BufferUsageFlag::RAYTRACING);
			if (!resize)
			{
				data.m_should_build = true;
			}
		}

		inline void ExecuteBuildASTask(Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, fg::RenderTaskHandle handle)
		{
			auto& data = fg.GetData<BuildASData>(handle);
			auto cmd_list = fg.GetCommandList(handle);
			auto context = rs.GetContext();
			auto model_pool = static_cast<gfx::VkModelPool*>(rs.GetModelPool());

			if (!data.m_should_build) return;

			std::vector<gfx::InstanceDesc> blas_instances;

			int material_id = 0;

			// Build blasses and create the instances list.
			for (auto const& batch : sg.GetRenderBatches())
			{
				auto model_handle = batch.m_model_handle;

				for (std::size_t i = 0; i < model_handle.m_mesh_handles.size(); i++)	
				{
					auto mesh_handle = model_handle.m_mesh_handles[i];

					auto blas = new gfx::AccelerationStructure(context);
					gfx::GeometryDesc geom_desc;

					geom_desc.m_indices_offset = mesh_handle.m_offsets.m_ib;
					geom_desc.m_vertices_offset = mesh_handle.m_offsets.m_vb;
					geom_desc.m_vertex_stride = mesh_handle.m_vertex_stride;
					geom_desc.m_num_indices = mesh_handle.m_num_indices;
					geom_desc.m_num_vertices = mesh_handle.m_num_vertices;
					geom_desc.m_ib = model_pool->m_big_index_buffer;
					geom_desc.m_vb = model_pool->m_big_vertex_buffer;

					//blas->SetScratchBuffer(data.m_scratch_buffer);
					blas->CreateBottomLevel(cmd_list, { geom_desc });
					data.m_blasses.push_back(blas);

					for (auto const & node_handle : batch.m_nodes)
					{
						auto node = sg.GetNode(node_handle);
						auto model_mat = glm::transpose(sg.m_models[node.m_transform_component].m_value);

						RaytracingOffset offset;
						offset.m_vertex_offset = geom_desc.m_vertices_offset;
						offset.m_index_offset = geom_desc.m_indices_offset;
						data.m_offsets[material_id] = offset;

						RaytracingMaterial material;
						material.m_albedo_texture = batch.m_material_handles[i].m_albedo_texture_handle;
						material.m_normal_texture = batch.m_material_handles[i].m_normal_texture_handle;
						material.m_roughness_texture = batch.m_material_handles[i].m_roughness_texture_handle;
						data.m_materials[material_id] = material;

						gfx::InstanceDesc new_instance;
						new_instance.m_transform = (glm::mat3x4)(model_mat);
						new_instance.m_blas = blas;
						new_instance.m_material = material_id;
						blas_instances.push_back(new_instance);

						material_id++;
					}
				}
			}

			// Fill the offsets buffer
			data.m_offsets_buffer->Map();
			data.m_offsets_buffer->Update(data.m_offsets.data(), data.m_offsets.size() * sizeof(RaytracingOffset));
			data.m_offsets_buffer->Unmap();

			// Fill the materials buffer
			data.m_materials_buffer->Map();
			data.m_materials_buffer->Update(data.m_materials.data(), data.m_materials.size() * sizeof(RaytracingMaterial));
			data.m_materials_buffer->Unmap();

			// Create TLAS
			data.m_tlas = new gfx::AccelerationStructure(context);
			data.m_tlas->CreateTopLevel(cmd_list, blas_instances);

			data.m_should_build = false;
		}

		inline void DestroyBuildASTask(fg::FrameGraph& fg, fg::RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<BuildASData>(handle);

			if (!resize)
			{
				for (auto& blas : data.m_blasses)
				{
					delete blas;
				}

				delete data.m_tlas;
				delete data.m_materials_buffer;
				delete data.m_offsets_buffer;
			}
		}

	} /* internal */

	inline void AddBuildASTask(fg::FrameGraph& fg)
	{
		fg::RenderTaskDesc desc;
		desc.m_setup_func = [](Renderer& rs, fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::SetupBuildASTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](Renderer& rs, fg::FrameGraph& fg, sg::SceneGraph& sg, ::fg::RenderTaskHandle handle)
		{
			internal::ExecuteBuildASTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](fg::FrameGraph& fg, ::fg::RenderTaskHandle handle, bool resize)
		{
			internal::DestroyBuildASTask(fg, handle, resize);
		};

		desc.m_properties = std::nullopt;
		desc.m_type = fg::RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<BuildASData>(desc, L"Build Acceleration Structures Task");
	}

} /* tasks */
