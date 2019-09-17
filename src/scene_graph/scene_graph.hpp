/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>
#include <cstdint>
#include <functional>
#include <typeindex>
#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/matrix_transform.hpp>

namespace sg
{

	using NodeHandle = std::uint32_t;
	using ComponentHandle = std::int32_t;

	static std::vector<std::function<void()>> component_create_func_table;

	// Build in components.
	class MeshComponent
	{
	};

	class TransformComponent
	{
	};

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
		ComponentData() = default;
		ComponentData(T value) { m_value = value; }
		operator T() const { return m_value; }
		T m_value;
	};

	class SceneGraph
	{
	public:
		SceneGraph();
		~SceneGraph() = default;

		NodeHandle CreateNode();

		void DestroyNode(NodeHandle handle);
		Node GetNode(NodeHandle handle);

		template<typename A, typename B>
		using Void_IsComponent = std::enable_if<std::is_same<A, B>::value, void>;

		template<typename T>
		typename Void_IsComponent<T, MeshComponent>::type PromoteNode(NodeHandle handle);

		template<typename T>
		typename Void_IsComponent<T, TransformComponent>::type PromoteNode(NodeHandle handle);

		void Update();

		// Transformation Component
		std::vector<ComponentData<glm::vec3>> m_positions;
		std::vector<ComponentData<glm::quat>> m_rotations;
		std::vector<ComponentData<glm::vec3>> m_scales;
		std::vector<ComponentData<glm::mat4>> m_models;
		std::vector<ComponentData<bool>> m_requires_update;

		// Mesh Component;
		std::vector<ComponentData<int>> m_meshes;

	private:
		std::vector<Node> m_nodes;

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
		m_positions.emplace_back(glm::vec3(0, 0, 0));
		m_rotations.emplace_back(glm::quat(0, 0, 0, 0));
		m_scales.emplace_back(glm::vec3(1, 1, 1));
		m_models.emplace_back(glm::mat4(1));
		m_requires_update.emplace_back(false);
	}

	namespace helper
	{
		inline void SetScale(SceneGraph* sg, NodeHandle handle, glm::vec3 value)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_scales[transform_handle] = value;
			sg->m_requires_update[transform_handle] = true;
		}

		inline void SetRotation(SceneGraph* sg, NodeHandle handle, glm::vec3 euler)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_rotations[transform_handle].m_value = glm::quat(euler);
			sg->m_requires_update[transform_handle] = true;
		}

	} /* helper */

} /* sg */