/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "scene_graph.hpp"

sg::SceneGraph::SceneGraph()
{

}

sg::NodeHandle sg::SceneGraph::CreateNode()
{
	NodeHandle new_node_handle = m_nodes.size();

	m_nodes.emplace_back(Node{
		.m_transform_component = -1,
		.m_mesh_component = -1
	});

	return new_node_handle;
}

void sg::SceneGraph::Update()
{
	// Transform Component
	for (std::size_t i = 0; i < m_requires_update.size(); i++)
	{
		if (!m_requires_update[i]) continue;

		auto& model = m_models[i].m_value;

		model = glm::mat4(1);
		model = glm::translate(model, m_positions[i].m_value);
		model = glm::scale(model, m_scales[i].m_value);
		model = glm::mat4_cast(m_rotations[i].m_value) * model;

		m_requires_update[i] = false;
	}

}

void sg::SceneGraph::DestroyNode(NodeHandle handle)
{

}

sg::Node sg::SceneGraph::GetNode(sg::NodeHandle handle)
{
	return m_nodes[handle];
}
