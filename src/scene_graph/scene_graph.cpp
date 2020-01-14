/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "scene_graph.hpp"

#include "../util/bitfield.hpp"
#include "../renderer.hpp"

sg::SceneGraph::SceneGraph(Renderer* renderer)
{
	m_meshes_require_batching.resize(gfx::settings::num_back_buffers);
	m_batch_requires_update.resize(gfx::settings::num_back_buffers);
	m_num_lights.resize(gfx::settings::num_back_buffers, 0);

	m_per_object_buffer_pool = renderer->CreateConstantBufferPool(sizeof(cb::Basic) * gfx::settings::max_render_batch_size, 100, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_TASK_BIT_NV);
	m_camera_buffer_pool = renderer->CreateConstantBufferPool(sizeof(cb::Camera), 1, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_TASK_BIT_NV);
	m_inverse_camera_buffer_pool = renderer->CreateConstantBufferPool(sizeof(cb::RaytracingCamera), 1, 2, VK_SHADER_STAGE_RAYGEN_BIT_NV);
	m_light_buffer_pool = renderer->CreateConstantBufferPool(sizeof(cb::Light) * gfx::settings::max_lights, 1, 3, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV);

	// Initialize the light buffer as empty.
	cb::Light light = {};
	light.m_type &= 3;
	light.m_type |= 0 << 2;
	m_light_buffer_handle = m_light_buffer_pool->Allocate(sizeof(cb::Light) * gfx::settings::max_lights);
	for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
	{
		m_light_buffer_pool->Update(m_light_buffer_handle, sizeof(cb::Light), &light, i);
	}
}

sg::SceneGraph::~SceneGraph()
{
	delete m_per_object_buffer_pool;
	delete m_camera_buffer_pool;
	delete m_inverse_camera_buffer_pool;
	delete m_light_buffer_pool;
}

sg::NodeHandle sg::SceneGraph::CreateNode()
{
	NodeHandle new_node_handle = m_nodes.size();

	m_nodes.emplace_back(Node{
		.m_transform_component = -1,
		.m_mesh_component = -1,
		.m_camera_component = -1,
		.m_light_component = -1
	});

	m_node_handles.push_back(new_node_handle);
	return new_node_handle;
}

void sg::SceneGraph::Update(std::uint32_t frame_idx)
{
	// Transform Component
	for (std::size_t i = 0; i < m_requires_update.size(); i++) // TODO: Using I is not safe. Should use the node handle component from compenent data.
	{
		if (!m_requires_update[i]) continue;

		auto& model = m_models[i].m_value;

		auto const & scale = m_scales[i].m_value;

		model = glm::mat4(scale.x, 0, 0, 0,
		                  0, scale.y, 0, 0,
		                  0, 0, scale.z, 0,
		                  0, 0, 0, 1);

		model = glm::mat4_cast(glm::quat(m_rotations[i].m_value)) * model;

		auto const & pos = m_positions[i].m_value;

		model = glm::mat4(1, 0, 0, 0,
		                  0, 1, 0, 0,
		                  0, 0, 1, 0,
		                  pos.x, pos.y, pos.z, 1) * model;

		m_requires_update[i] = false;

		// If this transform has a mesh component make sure it updates the constant buffers
		auto parent_node = m_nodes[m_requires_update[i].m_node_handle];
		if (parent_node.m_mesh_component != -1)
		{
			m_requires_buffer_update[parent_node.m_mesh_component] = std::vector<bool>(gfx::settings::num_back_buffers,true);
		}
		if (parent_node.m_camera_component != -1)
		{
			m_requires_camera_buffer_update[parent_node.m_camera_component] = std::vector<bool>(gfx::settings::num_back_buffers, true);
		}
		if (parent_node.m_light_component != -1)
		{
			m_requires_light_buffer_update[parent_node.m_light_component] = std::vector<bool>(gfx::settings::num_back_buffers, true);
		}
	}

	// Update constant bufffers for cameras
	for (auto& requires_update : m_requires_camera_buffer_update)
	{
		if (!requires_update.m_value[frame_idx]) continue;

		auto node = m_nodes[requires_update.m_node_handle];

		glm::vec3 cam_pos = m_positions[node.m_transform_component].m_value;
		glm::vec3 cam_rot = m_rotations[node.m_transform_component].m_value;

		glm::vec3 forward;
		forward.x = cos(cam_rot.y) * cos(cam_rot.x);
		forward.y = sin(cam_rot.x);
		forward.z = sin(cam_rot.y) * cos(cam_rot.x);
		forward = glm::normalize(forward);
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));

		float aspect_ratio = m_camera_aspect_ratios[node.m_camera_component].m_value;

		auto lens_properties = m_camera_lens_properties[node.m_camera_component].m_value;

		float vertical_size = lens_properties.m_film_size / aspect_ratio;
		float fov = lens_properties.m_fov;
		if (!lens_properties.m_use_simple_fov)
		{
			fov = 2.0f * std::atan2(vertical_size, 2.0f * lens_properties.m_focal_length);
		}

		cb::Camera data;
		data.m_view = glm::lookAt(cam_pos, cam_pos + forward, up);
		data.m_proj = glm::perspective(glm::radians(fov), aspect_ratio, 0.01f, 1000.0f);
		data.m_proj[1][1] *= -1;

		// TODO: In theory right now the cb handle and the mesh component will always have the same value.
		m_camera_buffer_pool->Update(m_camera_cb_handles[node.m_camera_component], sizeof(cb::Camera), &data, frame_idx);

		// Update inverse
		cb::RaytracingCamera inv_data;
		//inv_data.m_view = glm::inverse(data.m_view);
		//inv_data.m_proj = glm::inverse(data.m_proj);

		inv_data.cameraPositionAspect.x = cam_pos.x;
		inv_data.cameraPositionAspect.y = cam_pos.y;
		inv_data.cameraPositionAspect.z = cam_pos.z;
		inv_data.cameraPositionAspect.a = aspect_ratio;

		inv_data.cameraUpVectorTanHalfFOV.x = up.x;
		inv_data.cameraUpVectorTanHalfFOV.y = up.y;
		inv_data.cameraUpVectorTanHalfFOV.z = up.z;
		inv_data.cameraUpVectorTanHalfFOV.a = std::tan(0.5f * glm::radians(fov));;

		inv_data.cameraRightVectorLensR.x = right.x;
		inv_data.cameraRightVectorLensR.y = right.y;
		inv_data.cameraRightVectorLensR.z = right.z;
		inv_data.cameraRightVectorLensR.a = 0.5f * lens_properties.m_diameter;

		inv_data.cameraForwardVectorLensF.x = forward.x;
		inv_data.cameraForwardVectorLensF.y = forward.y;
		inv_data.cameraForwardVectorLensF.z = forward.z;
		inv_data.cameraForwardVectorLensF.a = lens_properties.m_focal_dist;

		m_inverse_camera_buffer_pool->Update(m_inverse_camera_cb_handles[node.m_camera_component], sizeof(cb::RaytracingCamera), &inv_data, frame_idx);

		m_requires_camera_buffer_update[node.m_camera_component].m_value[frame_idx] = false;
	}

	// Update constant bufffer for lights
	for (auto& requires_update : m_requires_light_buffer_update)
	{
		if (!requires_update.m_value[frame_idx]) continue;

		auto node = m_nodes[requires_update.m_node_handle];

		auto pos = m_positions[node.m_transform_component].m_value;
		auto rot = m_rotations[node.m_transform_component].m_value;
		auto color = m_colors[node.m_light_component].m_value;
		auto type = m_light_types[node.m_light_component].m_value;
		auto radius = m_radius[node.m_light_component].m_value;
		auto physical_size = m_light_physical_size[node.m_light_component].m_value;
		auto angles = m_light_angles[node.m_light_component].m_value;

		auto model = m_models[node.m_transform_component].m_value;

		glm::vec3 forward;
		forward.x = cos(rot.y) * cos(rot.x);
		forward.y = sin(rot.x);
		forward.z = sin(rot.y) * cos(rot.x);
		forward = glm::normalize(forward);

		cb::Light light;
		light.m_pos = pos;
		light.m_radius = radius;
		light.m_direction = forward;
		light.m_type = (std::uint32_t)type;
		light.m_inner_angle = angles.first;
		light.m_outer_angle = angles.second;
		light.m_physical_size = physical_size;
		if (node.m_light_component == 0)
		{
			light.m_type |= util::Pack(2, 30, (std::uint32_t)m_light_node_handles.size());
			m_num_lights[frame_idx] = m_light_node_handles.size(); // no need to update the size twice.
		}
		light.m_color = color;

		auto light_id = node.m_light_component;
		auto offset = light_id * (sizeof(cb::Light) + sizeof(glm::vec4)); // TODO: fix this random padding?

		m_light_buffer_pool->Update(m_light_buffer_handle, sizeof(cb::Light), &light, frame_idx, offset);

		m_requires_light_buffer_update[node.m_light_component].m_value[frame_idx] = false;
	}

	// A light was added or removed
	if (m_num_lights[frame_idx] != m_light_node_handles.size())
	{
		cb::Light light{};
		if (!m_light_node_handles.empty())
		{
			auto node = m_nodes[m_light_node_handles[0]];

			auto pos = m_positions[node.m_transform_component].m_value;
			auto rot = m_rotations[node.m_transform_component].m_value;
			auto color = m_colors[node.m_light_component].m_value;
			auto type = m_light_types[node.m_light_component].m_value;
			auto radius = m_radius[node.m_light_component].m_value;
			auto physical_size = m_light_physical_size[node.m_light_component].m_value;
			auto angles = m_light_angles[node.m_light_component].m_value;

			glm::vec3 forward;
			forward.x = cos(rot.y) * cos(rot.x);
			forward.y = sin(rot.x);
			forward.z = sin(rot.y) * cos(rot.x);
			forward = glm::normalize(forward);

			light.m_pos = pos;
			light.m_radius = radius;
			light.m_physical_size = physical_size;
			light.m_direction = forward;

			light.m_inner_angle = angles.first;
			light.m_outer_angle = angles.second;
			light.m_color = color;

			light.m_type = (std::uint32_t)type;
		}
		light.m_type |= util::Pack(2, 30, (std::uint32_t)m_light_node_handles.size());

		m_light_buffer_pool->Update(m_light_buffer_handle, sizeof(cb::Light), &light, frame_idx, 0);

		m_num_lights[frame_idx] = m_light_node_handles.size();
	}

	// Generate Batches
	for (auto& node_handle : m_meshes_require_batching[0])
	{
		auto node = m_nodes[node_handle];
		auto model_handle = m_model_handles[node.m_mesh_component].m_value;
		auto material_handles = m_model_material_handles[node.m_mesh_component].m_value;

		bool batch_available = false;
		std::size_t batch_idx = 0;
		for (auto& batch : m_render_batches)
		{
			if (FitsInBatch(batch, node))
			{
				batch_available = true;
				batch.m_num_meshes++;
				batch.m_nodes.push_back(node_handle);

				for (auto idx = 0; idx < gfx::settings::num_back_buffers; idx++)
				{
					m_batch_requires_update[idx].push_back({ batch_idx, node_handle });
				}

				break;
			}

			batch_idx++;
		}

		if (!batch_available)
		{
			RenderBatch new_batch;
			new_batch.m_model_handle = model_handle;
			new_batch.m_material_handles = material_handles;
			new_batch.m_num_meshes = 1;
			new_batch.m_nodes.push_back(node_handle);
			new_batch.m_big_cb = m_per_object_buffer_pool->Allocate(sizeof(cb::Basic) * gfx::settings::max_render_batch_size);

			m_render_batches.push_back(new_batch);

			for (auto idx = 0; idx < gfx::settings::num_back_buffers; idx++)
			{
				m_batch_requires_update[idx].push_back({ m_render_batches.size() - 1, node_handle });
			}
		}
	}
	m_meshes_require_batching[0].clear();

	// Efficient batch update. (new/delete)
	for (auto& batch_handle_pair : m_batch_requires_update[frame_idx])
	{
		auto node = m_nodes[batch_handle_pair.second];
		auto batch = m_render_batches[batch_handle_pair.first];

		cb::Basic data;
		data.m_model = m_models[node.m_transform_component].m_value;

		// Find the position of the mesh that requires a update inside of the constant buffer.
		int update_offset = 0;
		for (auto batch_node_handle : batch.m_nodes)
		{
			if (batch_node_handle == batch_handle_pair.second)
			{
				break;
			}

			update_offset++;
		}

		// TODO: In theory right now the cb handle and the mesh component will always have the same value.
		m_per_object_buffer_pool->Update(batch.m_big_cb, sizeof(cb::Basic), &data, frame_idx, update_offset * sizeof(cb::Basic));

		m_requires_buffer_update[node.m_mesh_component].m_value[frame_idx] = false;
	}
	m_batch_requires_update[frame_idx].clear();

	// Update batch cb in case a mesh was moved
	for (auto& requires_update : m_requires_buffer_update)
	{
		if (!requires_update.m_value[frame_idx]) continue;

		auto node_handle = requires_update.m_node_handle;
		auto node = m_nodes[node_handle];
		auto model_mat = m_models[node.m_transform_component].m_value;
		auto model_handle = m_model_handles[node.m_mesh_component].m_value;

		// find appropriate batch
		bool updated_batch = false;
		for (auto& batch : m_render_batches)
		{
			if (FitsInBatch(batch, node))
			{
				// Find the position of the mesh that requires a update inside of the constant buffer.
				int update_offset = 0;
				for (auto batch_node_handle : batch.m_nodes)
				{
					if (batch_node_handle == node_handle)
					{
						break;
					}

					update_offset++;
				}

				cb::Basic data;
				data.m_model = model_mat;
				m_per_object_buffer_pool->Update(batch.m_big_cb, sizeof(cb::Basic), &data, frame_idx, update_offset * sizeof(cb::Basic));

				updated_batch = true;
				requires_update.m_value[frame_idx] = false;
				break;
			}
		}

		if (!updated_batch)
		{
			LOGW("Mesh required update but failed to update it...");
		}
	}
}

sg::Node sg::SceneGraph::GetActiveCamera()
{
	return m_nodes[m_camera_node_handles[0]];
}

ConstantBufferPool* sg::SceneGraph::GetPOConstantBufferPool()
{
	return m_per_object_buffer_pool;
}

ConstantBufferPool* sg::SceneGraph::GetCameraConstantBufferPool()
{
	return m_camera_buffer_pool;
}

ConstantBufferPool* sg::SceneGraph::GetInverseCameraConstantBufferPool()
{
	return m_inverse_camera_buffer_pool;
}

ConstantBufferPool* sg::SceneGraph::GetLightConstantBufferPool()
{
	return m_light_buffer_pool;
}

ConstantBufferHandle sg::SceneGraph::GetLightBufferHandle()
{
	return m_light_buffer_handle;
}

sg::Node sg::SceneGraph::GetNode(sg::NodeHandle handle)
{
	return m_nodes[handle];
}

std::vector<sg::Node> const& sg::SceneGraph::GetNodes() const
{
	return m_nodes;
}

std::vector<sg::NodeHandle> const & sg::SceneGraph::GetNodeHandles() const
{
	return m_node_handles;
}

std::vector<sg::NodeHandle> const & sg::SceneGraph::GetMeshNodeHandles() const
{
	return m_mesh_node_handles;
}

std::vector<sg::RenderBatch> const& sg::SceneGraph::GetRenderBatches() const
{
	return m_render_batches;
}
