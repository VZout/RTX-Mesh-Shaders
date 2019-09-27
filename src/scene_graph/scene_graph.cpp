/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "scene_graph.hpp"

sg::SceneGraph::SceneGraph()
{
	m_num_lights.resize(gfx::settings::num_back_buffers, 0);
}

sg::SceneGraph::~SceneGraph()
{
	delete m_per_object_buffer_pool;
	delete m_camera_buffer_pool;
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

		model = glm::mat4(1);
		model = glm::scale(glm::mat4(1), m_scales[i].m_value) * model;
		model = glm::mat4_cast(m_rotations[i].m_value) * model;
		model = glm::translate(glm::mat4(1), m_positions[i].m_value) * model;

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

	// Update constant bufffers for transformations (Meshes only)
	for (auto& requires_update : m_requires_buffer_update)
	{
		if (!requires_update.m_value[frame_idx]) continue;

		auto node = m_nodes[requires_update.m_node_handle];
		auto model_mat = m_models[node.m_transform_component].m_value;

		cb::Basic data;
		data.m_model = model_mat;

		// TODO: In theory right now the cb handle and the mesh component will always have the same value.
		m_per_object_buffer_pool->Update(m_transform_cb_handles[node.m_mesh_component], sizeof(cb::Basic), &data, frame_idx);

		m_requires_buffer_update[node.m_mesh_component].m_value[frame_idx] = false;
	}

	// Update constant bufffers for cameras
	for (auto& requires_update : m_requires_camera_buffer_update)
	{
		if (!requires_update.m_value[frame_idx]) continue;

		auto node = m_nodes[requires_update.m_node_handle];

		glm::vec3 cam_pos = m_positions[node.m_transform_component].m_value;
		glm::quat cam_rot = m_rotations[node.m_transform_component].m_value;

		glm::mat4 rotate = glm::mat4_cast(cam_rot);
		glm::mat4 translate = glm::translate(glm::mat4(1), cam_pos);

		cb::Camera data;
		data.m_view = rotate * translate;
		data.m_proj = glm::perspective(glm::radians(70.0f), (float) 1280 / (float) 720, 0.01f, 1000.0f);
		data.m_proj[1][1] *= -1;

		// TODO: In theory right now the cb handle and the mesh component will always have the same value.
		m_camera_buffer_pool->Update(m_camera_cb_handles[node.m_camera_component], sizeof(cb::Camera), &data, frame_idx);

		m_requires_camera_buffer_update[node.m_camera_component].m_value[frame_idx] = false;
	}

	// Update constant bufffer for lights
	for (auto& requires_update : m_requires_light_buffer_update)
	{
		if (!requires_update.m_value[frame_idx]) continue;

		auto node = m_nodes[requires_update.m_node_handle];

		glm::vec3 pos = m_positions[node.m_transform_component].m_value;
		glm::vec3 color = m_colors[node.m_light_component].m_value;

		cb::Light light;
		light.m_pos = pos;
		light.m_type = (std::uint32_t)cb::LightType::POINT;
		if (node.m_light_component == 0)
		{
			light.m_type &= 0x3; // Keep id
			light.m_type |= std::uint32_t(m_light_node_handles.size() + 1) << 2; // Set number of lights
		}
		light.m_color = color;

		auto light_id = node.m_light_component;
		auto offset = light_id * sizeof(cb::Light);

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
			light.m_pos = m_positions[node.m_transform_component].m_value;
			light.m_color = m_colors[node.m_light_component].m_value;
			light.m_type = (std::uint32_t)cb::LightType::POINT;
		}
		light.m_type &= 0x3; // Keep id
		light.m_type |= std::uint32_t(m_light_node_handles.size() + 1) << 2; // Set number of lights

		m_light_buffer_pool->Update(m_light_buffer_handle, sizeof(cb::Light), &light, frame_idx, 0);

		m_num_lights[frame_idx] = m_light_node_handles.size();
	}
}

void sg::SceneGraph::SetPOConstantBufferPool(ConstantBufferPool* pool)
{
	m_per_object_buffer_pool = pool;
}

ConstantBufferPool* sg::SceneGraph::GetPOConstantBufferPool()
{
	return m_per_object_buffer_pool;
}

void sg::SceneGraph::SetCameraConstantBufferPool(ConstantBufferPool* pool)
{
	m_camera_buffer_pool = pool;
}

ConstantBufferPool* sg::SceneGraph::GetCameraConstantBufferPool()
{
	return m_camera_buffer_pool;
}

void sg::SceneGraph::SetLightConstantBufferPool(ConstantBufferPool* pool)
{
	m_light_buffer_pool = pool;
	m_light_buffer_handle = m_light_buffer_pool->Allocate(sizeof(cb::Light) * gfx::settings::max_lights);

	cb::Light light = {};
	light.m_type &= 3;
	light.m_type |= 0 << 2;

	// Initialize the buffer as empty.
	for (std::uint32_t i = 0; i < gfx::settings::num_back_buffers; i++)
	{
		m_light_buffer_pool->Update(m_light_buffer_handle, sizeof(cb::Light), &light, i);
	}
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

std::vector<sg::NodeHandle> const & sg::SceneGraph::GetNodeHandles() const
{
	return m_node_handles;
}

std::vector<sg::NodeHandle> const & sg::SceneGraph::GetMeshNodeHandles() const
{
	return m_mesh_node_handles;
}
