/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

class Renderer;

namespace gfx
{

	class Context;
	class RootSignature;
	class GPUBuffer;
	class StagingTexture;
	class RenderTarget;

	class DescHeapHandle
	{
		VkDescriptorSet m_descriptor_set;
	};

	class DescriptorHeap
	{
		friend class CommandList;

		friend class ::Renderer; // TODO: remove me bich
	public:
		struct Desc
		{
			std::uint32_t m_num_descriptors = 1;
			std::uint32_t m_versions = 1;
		};

		explicit DescriptorHeap(Context* context, Desc desc);
		~DescriptorHeap();

		std::uint32_t CreateSRVFromCB(GPUBuffer* buffer, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx);
		std::uint32_t CreateSRVFromTexture(StagingTexture* texture, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx);
		std::uint32_t CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx);
		std::uint32_t CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, VkDescriptorSetLayout layout, std::uint32_t handle, std::uint32_t frame_idx);
		std::uint32_t CreateSRVSetFromRT(RenderTarget* render_target, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx, bool include_depth = true);

	private:
		Context* m_context;

		Desc m_desc;
		VkDescriptorPoolCreateInfo m_descriptor_pool_create_info;
		std::vector<VkDescriptorPool> m_descriptor_pools;
		std::vector<std::vector<VkDescriptorSet>> m_descriptor_sets; // first array is versions second array is sets
		std::vector<VkImageView> m_image_views; // stores image views for textures.
		std::vector<VkSampler> m_image_samplers; // store sampler for textures.
	};

} /* gfx */