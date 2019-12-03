/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "vulkan/vulkan.h"

#include <vector>
#include <cstdint>

#include "command_queue.hpp"

class ImGuiImpl;

namespace gfx
{
	class Context;
	class RenderTarget;
	class RenderWindow;
	class RootSignature;
	class DescriptorHeap;
	class PipelineState;
	class StagingTexture;
	class StagingBuffer;
	class Texture;
	class ShaderTable;

	class CommandList
	{
		friend class RenderWindow;
		friend class CommandQueue;
		friend class ::ImGuiImpl;
		friend class AccelerationStructure;
	public:
		explicit CommandList(CommandQueue* queue);
		~CommandList();

		void Begin(std::uint32_t frame_idx);
		void Close();

		void BindRenderTargetVersioned(RenderTarget* render_target);
		void BindRenderTarget(RenderTarget* render_target);
		void UnbindRenderTarget();
		void BindPipelineState(PipelineState* pipeline);
		void BindVertexBuffer(GPUBuffer* staging_buffer, std::uint64_t offset = 0);
		void BindIndexBuffer(GPUBuffer* staging_buffer, std::uint64_t stride, std::uint64_t offset = 0);
		void BindDescriptorHeap(RootSignature* root_signature, std::vector<std::pair<DescriptorHeap*, std::uint32_t>> sets);
		void BindComputePushConstants(RootSignature* root_signature, void* data, std::uint32_t size);
		void BindTaskPushConstants(RootSignature* root_signature, void* data, std::uint32_t size);
		void StageBuffer(StagingBuffer* staging_buffer);
		void StageBuffer(GPUBuffer* buffer, GPUBuffer* staging_buffer, std::uint64_t offset);
		void StageTexture(StagingTexture* texture);
		void CopyRenderTargetToRenderWindow(RenderTarget* render_target, std::uint32_t rt_idx, RenderWindow* render_window);
		void TransitionDepth(RenderTarget* render_target, VkImageLayout from, VkImageLayout to);
		void TransitionTexture(StagingTexture* texture, VkImageLayout from, VkImageLayout to);
		void TransitionRenderTarget(RenderTarget* render_target, VkImageLayout from, VkImageLayout to);
		void TransitionRenderTarget(RenderTarget* render_target, std::uint32_t rt_idx, VkImageLayout from, VkImageLayout to);
		void GenerateMipMap(gfx::Texture* texture);
		void GenerateMipMap(gfx::RenderTarget* render_target);
		void GenerateMipMap(VkImage& image, VkFormat format, std::int32_t width, std::int32_t height, std::uint32_t mip_levels, std::uint32_t layers);
		void Draw(std::uint32_t vertex_count, std::uint32_t instance_count,
				std::uint32_t first_vertex = 0, std::uint32_t first_instance = 0);
		void DrawIndexed(std::uint32_t idx_count, std::uint32_t instance_count,
				std::uint32_t first_idx = 0, std::uint32_t vertex_offset = 0, std::uint32_t first_instance = 0);
		void Dispatch(std::uint32_t tg_count_x, std::uint32_t tg_count_y, std::uint32_t tg_count_z);
		void DispatchRays(
			ShaderTable* raygen_table, ShaderTable* miss_table, ShaderTable* hit_table,
			std::uint32_t width, std::uint32_t height, std::uint32_t depth);
		void DrawMesh(std::uint32_t count, std::uint32_t first);

	private:
		Context* m_context;
		CommandQueue* m_queue;

		VkCommandPool m_cmd_pool;
		VkCommandPoolCreateInfo m_cmd_pool_create_info;
		std::vector<VkCommandBuffer> m_cmd_buffers;

		std::uint32_t m_frame_idx;
		VkPipelineBindPoint m_current_bind_point;
	};

} /* gfx */
