#pragma once

#include <vector>
#include <cstdint>
#include <functional>
#include <typeindex>

using NodeHandle = std::uint32_t;
using ComponentHandle = std::int32_t;

static std::vector<std::function<void()>> component_create_func_table;

// Build in components.
class MeshComponent { };
class TransformComponent { };

struct Node
{
	ComponentHandle m_transform_component;
	ComponentHandle m_mesh_component;
};

namespace internal
{

	struct BaseComponentData
	{
		NodeHandle m_node_handle;
	};

} /* internal */

template<typename T>
struct ComponentData : internal::BaseComponentData
{
	T m_value;
};

class SceneGraph
{
	public:
		SceneGraph();
		~SceneGraph() = default;

		NodeHandle CreateNode();
		void DestroyNode(NodeHandle handle);

		template<typename A, typename B>
		using Void_IsComponent = std::enable_if<std::is_same<A, B>::value, void>;

		template<typename T>
		typename Void_IsComponent<T, MeshComponent>::type PromoteNode(NodeHandle handle);

		template<typename T>
		typename Void_IsComponent<T, TransformComponent>::type PromoteNode(NodeHandle handle);

	private:
		std::vector<Node> m_nodes;

		std::vector<ComponentData<float[3]>> m_positions;
		std::vector<ComponentData<float[3]>> m_rotations;
		std::vector<ComponentData<int>> m_meshes;
};

template<typename T>
typename SceneGraph::Void_IsComponent<T, MeshComponent>::type SceneGraph::PromoteNode(NodeHandle handle)
{
	auto& node = m_nodes[handle];
	node.m_mesh_component = m_meshes.size();

	if (node.m_transform_component == -1)
	{
		PromoteNode<TransformComponent>(handle);
	}

	ComponentData<int> mesh_data = {};
	mesh_data.m_value = handle;
	mesh_data.m_node_handle = handle;

	m_meshes.push_back(mesh_data);
}

template<typename T>
typename SceneGraph::Void_IsComponent<T, TransformComponent>::type SceneGraph::PromoteNode(NodeHandle handle)
{
	auto& node = m_nodes[handle];
	node.m_transform_component = m_positions.size();
}