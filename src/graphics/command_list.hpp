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

	class CommandList
	{
		friend class RenderWindow;
		friend class CommandQueue;
		friend class ::ImGuiImpl;
	public:
		explicit CommandList(CommandQueue* queue);
		~CommandList();

		void Begin(std::uint32_t frame_idx);
		void Close();

		void BindRenderTargetVersioned(RenderTarget* render_target);
		void BindRenderTarget(RenderTarget* render_target);
		void UnbindRenderTarget();
		void BindPipelineState(PipelineState* pipeline);
		void BindComputePipelineState(PipelineState* pipeline);
		void BindVertexBuffer(StagingBuffer* staging_buffer);
		void BindIndexBuffer(StagingBuffer* staging_buffer);
		void BindDescriptorHeap(RootSignature* root_signature, std::vector<std::pair<DescriptorHeap*, std::uint32_t>> sets);
		void BindComputeDescriptorHeap(RootSignature* root_signature, std::vector<std::pair<DescriptorHeap*, std::uint32_t>> sets);
		void BindComputePushConstants(RootSignature* root_signature, void* data, std::uint32_t size);
		void StageBuffer(StagingBuffer* staging_buffer);
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

	private:
		Context* m_context;
		CommandQueue* m_queue;

		VkCommandPool m_cmd_pool;
		VkCommandPoolCreateInfo m_cmd_pool_create_info;
		std::vector<VkCommandBuffer> m_cmd_buffers;

		std::uint32_t m_frame_idx;
	};

} /* gfx */
