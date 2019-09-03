#include "scene_graph.hpp"

SceneGraph::SceneGraph()
{

}

NodeHandle SceneGraph::CreateNode()
{
	NodeHandle new_node_handle = m_nodes.size();

	Node new_node
	{
		.m_transform_component  = -1,
		.m_mesh_component = -1
	};

	m_nodes.push_back({
		.m_transform_component = -1,
		.m_mesh_component = -1
	});

	return new_node_handle;
}

void SceneGraph::DestroyNode(NodeHandle handle)
{

}