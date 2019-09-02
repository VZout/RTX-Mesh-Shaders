#pragma once

#include <vector>
#include <cstdint>

using NodeHandle = std::uint32_t;
using ComponentHandle = std::int32_t;

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
}

using NodeHandle = std::size_t;
using NodeList = std::vector<Node>;

class SceneGraph
{
	public:
		std::pair<NodeHandle> CreateNode();
		void DestroyNode(NodeHandle handle);
		NodeList const & GetNodes();
		TransformList const & GetTransforms();
			
		// instead of this define 1 function to add or remove component. That component is a empty struct but that struct can be used to look into a table of create/remove functions so you can have component specific initialization.
		void PromotoToMesh();
		void RemoveMesh();

		void PromotoToLight();
		void RemoveLight();

	private:
		NodeList m_nodes;

		ComponentData<float[3]> m_positions;
		ComponentData<float[3]> m_rotations;
		ComponentData<void> m_meshes;
};
