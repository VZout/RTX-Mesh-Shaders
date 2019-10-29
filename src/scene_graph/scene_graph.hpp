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

#include "../model_pool.hpp"
#include "../util/delegate.hpp"
#include "../buffer_definitions.hpp"
#include "../constant_buffer_pool.hpp"
#include "../graphics/gfx_settings.hpp"

class Renderer;

namespace sg
{

	using NodeHandle = std::uint32_t;
	using ComponentHandle = std::int32_t;

	static std::vector<util::Delegate<void()>> component_create_func_table;

	// Build in components.
	class MeshComponent
	{
	};

	class TransformComponent
	{
	};

	class CameraComponent
	{
	};

	class LightComponent
	{
	};

	struct RenderBatch
	{
		std::uint32_t m_num_meshes;
		ModelHandle m_model_handle;
		std::vector<MaterialHandle> m_material_handles;
		std::vector<NodeHandle> m_nodes;
		ConstantBufferHandle m_big_cb;
	};

	struct Node
	{
		ComponentHandle m_transform_component;
		ComponentHandle m_mesh_component;
		ComponentHandle m_camera_component;
		ComponentHandle m_light_component;
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
		ComponentData(T value, NodeHandle handle) : BaseComponentData() { m_value = value; m_node_handle = handle; }
		void operator=(T const & value) {
			m_value = value;
		}

		operator T() const { return m_value; }
		T m_value;
	};

	class SceneGraph
	{
	public:
		SceneGraph(Renderer* renderer);
		~SceneGraph();

		NodeHandle CreateNode();

		template<typename T, typename... Args>
		NodeHandle CreateNode(Args... args)
		{
			auto handle = CreateNode();

			PromoteNode<T>(handle, args...);

			return handle;
		}

		Node GetNode(NodeHandle handle);
		std::vector<NodeHandle> const & GetNodeHandles() const;
		std::vector<NodeHandle> const & GetMeshNodeHandles() const;
		std::vector<RenderBatch> const& GetRenderBatches() const;

		template<typename A, typename B>
		using Void_IsComponent = std::enable_if<std::is_same<A, B>::value, void>;

		template<typename T>
		typename Void_IsComponent<T, MeshComponent>::type PromoteNode(NodeHandle handle, ModelHandle model_handle)
		{
			auto& node = m_nodes[handle];
			node.m_mesh_component = m_model_handles.size();

			if (node.m_transform_component == -1)
			{
				PromoteNode<TransformComponent>(handle);
			}

			m_model_handles.emplace_back(ComponentData<ModelHandle>(
				model_handle,
				handle
			));

			m_requires_buffer_update.emplace_back(ComponentData<std::vector<bool>>(
				std::vector<bool>(gfx::settings::num_back_buffers, true),
				handle
			));

			// TODO: Simplify this by moving the material handle from the mesh to the model.
			std::vector<MaterialHandle> mats;
			for (auto const & mesh_handle : model_handle.m_mesh_handles)
			{
				if (mesh_handle.m_material_handle.has_value())
				{
					mats.push_back(mesh_handle.m_material_handle.value());
				}
			}
			m_model_material_handles.emplace_back(ComponentData<std::vector<MaterialHandle>>(
				mats,
				handle
			));

			m_mesh_node_handles.push_back(handle);
			m_meshes_require_batching.push_back(handle);
		}

		template<typename T>
		typename Void_IsComponent<T, TransformComponent>::type PromoteNode(NodeHandle handle)
		{
			auto& node = m_nodes[handle];
			node.m_transform_component = m_positions.size();
			m_positions.emplace_back(glm::vec3(0, 0, 0), handle);
			m_rotations.emplace_back(glm::vec3(0, 0, 0), handle);
			m_scales.emplace_back(glm::vec3(1, 1, 1), handle);
			m_models.emplace_back(glm::mat4(1), handle);
			m_requires_update.emplace_back(false, handle);
		}

		template<typename T>
		typename Void_IsComponent<T, CameraComponent>::type PromoteNode(NodeHandle handle)
		{
			auto& node = m_nodes[handle];
			node.m_camera_component = m_camera_cb_handles.size();

			// A camera requires a transform.
			if (node.m_transform_component == -1)
			{
				PromoteNode<TransformComponent>(handle);
			}

			// Allocate constant buffer.
			auto camera_cb_handle = m_camera_buffer_pool->Allocate(sizeof(cb::Camera));
			m_camera_cb_handles.emplace_back(ComponentData<ConstantBufferHandle>(
				camera_cb_handle,
				handle
			));

			m_camera_aspect_ratios.emplace_back(ComponentData<float>(
				1280.f / 720.f,
				handle
			));

			m_requires_camera_buffer_update.emplace_back(ComponentData<std::vector<bool>>(
				std::vector<bool>(gfx::settings::num_back_buffers, true),
				handle
			));

			m_camera_node_handles.push_back(handle);
		}

		template<typename T>
		typename Void_IsComponent<T, LightComponent>::type PromoteNode(NodeHandle handle, cb::LightType type, glm::vec3 color = { 1, 1, 1 })
		{
			auto& node = m_nodes[handle];
			node.m_light_component = m_light_node_handles.size();

			// A light requires a transform.
			if (node.m_transform_component == -1)
			{
				PromoteNode<TransformComponent>(handle);
			}

			m_requires_light_buffer_update.emplace_back(ComponentData<std::vector<bool>>(
					std::vector<bool>(gfx::settings::num_back_buffers, true),
					handle
			));

			m_colors.emplace_back(ComponentData<glm::vec3>{color, handle});
			m_light_types.emplace_back(ComponentData<cb::LightType>{type, handle});
			m_radius.emplace_back(ComponentData<float>{0, handle});
			m_light_angles.emplace_back(ComponentData<std::pair<float, float>>{ {glm::radians(40.f), glm::radians(50.f)}, handle});

			m_light_node_handles.push_back(handle);
		}

		void Update(std::uint32_t frame_idx);

		Node GetActiveCamera();

		ConstantBufferPool* GetPOConstantBufferPool();
		ConstantBufferPool* GetCameraConstantBufferPool();
		ConstantBufferPool* GetLightConstantBufferPool();
		ConstantBufferHandle GetLightBufferHandle();

		// Transformation Component
		std::vector<ComponentData<glm::vec3>> m_positions;
		std::vector<ComponentData<glm::vec3>> m_rotations;
		std::vector<ComponentData<glm::vec3>> m_scales;
		std::vector<ComponentData<glm::mat4>> m_models;
		std::vector<ComponentData<bool>> m_requires_update;

		// Mesh Component
		std::vector<ComponentData<ModelHandle>> m_model_handles;
		std::vector<ComponentData<std::vector<MaterialHandle>>> m_model_material_handles;
		std::vector<ComponentData<std::vector<bool>>> m_requires_buffer_update;

		// Camera Component
		std::vector<ComponentData<ConstantBufferHandle>> m_camera_cb_handles;
		std::vector<ComponentData<float>> m_camera_aspect_ratios;
		std::vector<ComponentData<std::vector<bool>>> m_requires_camera_buffer_update;

		// Light Component
		std::vector<ComponentData<std::vector<bool>>> m_requires_light_buffer_update;
		std::vector<ComponentData<glm::vec3>> m_colors;
		std::vector<ComponentData<cb::LightType>> m_light_types;
		std::vector<ComponentData<float>> m_radius;
		std::vector<ComponentData<std::pair<float, float>>> m_light_angles;
		std::vector<std::size_t> m_num_lights;

		// Batching
		std::vector<NodeHandle> m_meshes_require_batching;
		std::vector<std::vector<std::pair<std::uint32_t, std::uint32_t>>> m_batch_requires_update;
		std::vector<RenderBatch> m_render_batches;

	private:
		std::vector<Node> m_nodes;
		std::vector<NodeHandle> m_node_handles;
		std::vector<NodeHandle> m_mesh_node_handles;
		std::vector<NodeHandle> m_camera_node_handles;
		std::vector<NodeHandle> m_light_node_handles;
		ConstantBufferPool* m_per_object_buffer_pool;
		ConstantBufferPool* m_camera_buffer_pool;
		ConstantBufferPool* m_light_buffer_pool;
		ConstantBufferHandle m_light_buffer_handle;

	};

	namespace helper
	{
		inline std::pair<glm::vec3, glm::vec3> GetForwardRight(SceneGraph* sg, NodeHandle handle)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			auto rot = sg->m_rotations[transform_handle].m_value;

			glm::vec3 forward;
			forward.x = cos(rot.y) * cos(rot.x);
			forward.y = sin(rot.x);
			forward.z = sin(rot.y) * cos(rot.x);
			forward = glm::normalize(forward);

			glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

			return { forward, right };
		}

		inline void SetMaterial(SceneGraph* sg, NodeHandle handle, std::vector<MaterialHandle> mats)
		{
			auto model_handle = sg->GetNode(handle).m_mesh_component;
			sg->m_model_material_handles[model_handle] = mats;
		}

		inline void Translate(SceneGraph* sg, NodeHandle handle, glm::vec3 value)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_positions[transform_handle].m_value += value;
			sg->m_requires_update[transform_handle] = true;
		}

		inline void SetPosition(SceneGraph* sg, NodeHandle handle, glm::vec3 value)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_positions[transform_handle].m_value = value;
			sg->m_requires_update[transform_handle] = true;
		}

		inline void SetScale(SceneGraph* sg, NodeHandle handle, glm::vec3 value)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_scales[transform_handle].m_value = value;
			sg->m_requires_update[transform_handle] = true;
		}

		inline void SetRotation(SceneGraph* sg, NodeHandle handle, glm::vec3 euler)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_rotations[transform_handle].m_value = euler;
			sg->m_requires_update[transform_handle] = true;
		}

		inline glm::vec3 GetRotation(SceneGraph* sg, NodeHandle handle)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			return sg->m_rotations[transform_handle].m_value;
		}

		inline void Rotate(SceneGraph* sg, NodeHandle handle, glm::vec3 euler)
		{
			auto transform_handle = sg->GetNode(handle).m_transform_component;
			sg->m_rotations[transform_handle].m_value += euler;
			sg->m_requires_update[transform_handle] = true;
		}

		inline void SetRadius(SceneGraph* sg, NodeHandle handle, float radius)
		{
			auto light_handle = sg->GetNode(handle).m_light_component;
			sg->m_radius[light_handle].m_value = radius;
			sg->m_requires_light_buffer_update[light_handle] = { true, true, true };
		}

		inline void SetAspectRatio(SceneGraph* sg, NodeHandle handle, float ratio)
		{
			auto camera_handle = sg->GetNode(handle).m_camera_component;
			sg->m_camera_aspect_ratios[camera_handle].m_value = ratio;
			sg->m_requires_camera_buffer_update[camera_handle] = { true, true, true };
		}

	} /* helper */

} /* sg */