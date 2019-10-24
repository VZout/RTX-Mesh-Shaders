/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <optional>

#include "gfx_enums.hpp"

class Renderer;

namespace gfx
{

	class Context;
	class RootSignature;
	class GPUBuffer;
	class Texture;
	class StagingTexture;
	class RenderTarget;

	struct SamplerDesc
	{
		enums::TextureFilter m_filter;
		enums::TextureAddressMode m_address_mode;
		enums::BorderColor m_border_color = enums::BorderColor::BORDER_WHITE;
	};

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

		inline static SamplerDesc m_default_sampler_desc
		{
			.m_filter = enums::TextureFilter::FILTER_LINEAR,
			.m_address_mode = enums::TextureAddressMode::TAM_WRAP,
			.m_border_color = enums::BorderColor::BORDER_WHITE,
		};

		VkDescriptorSet GetDescriptorSet(std::uint32_t frame_idx, std::uint32_t handle);

		std::uint32_t CreateSRVFromCB(GPUBuffer* buffer, VkDescriptorSetLayout layout, std::uint32_t handle, std::uint32_t frame_idx, bool uniform = true);
		std::uint32_t CreateSRVFromCB(GPUBuffer* buffer, RootSignature* root_signature, std::uint32_t handle, std::uint32_t frame_idx, bool uniform = true);
		std::uint32_t CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, RootSignature* root_signature,
				std::uint32_t handle, std::uint32_t frame_idx, std::optional<SamplerDesc> sampler_desc = m_default_sampler_desc);
		std::uint32_t CreateSRVSetFromTexture(std::vector<StagingTexture*> texture, VkDescriptorSetLayout layout, // TODO: Change this to texture instead of staging texture.
				std::uint32_t handle, std::uint32_t frame_idx, std::optional<SamplerDesc> sampler_desc = m_default_sampler_desc);
		std::uint32_t CreateUAVSetFromTexture(std::vector<Texture*> texture, RootSignature* root_signature,
				std::uint32_t handle, std::uint32_t frame_idx, std::optional<SamplerDesc> sampler_desc = m_default_sampler_desc);
		std::uint32_t CreateSRVSetFromRT(RenderTarget* render_target, RootSignature* root_signature,
				std::uint32_t handle,std::uint32_t frame_idx, bool include_depth = true, std::optional<SamplerDesc> sampler_desc = m_default_sampler_desc);
		std::uint32_t CreateSRVSetFromRT(RenderTarget* render_target, VkDescriptorSetLayout layout,
			std::uint32_t handle, std::uint32_t frame_idx, bool include_depth = true, std::optional<SamplerDesc> sampler_desc = m_default_sampler_desc);
		std::uint32_t CreateUAVSetFromRT(RenderTarget* render_target, std::uint32_t rt_idx, RootSignature* root_signature,
		                                 std::uint32_t handle,std::uint32_t frame_idx, SamplerDesc sampler_desc = m_default_sampler_desc, std::optional<float> mip_level = std::nullopt);

	private:
		VkSampler CreateSampler(SamplerDesc sampler, std::uint32_t num_mips = 1);

		Context* m_context;

		Desc m_desc;
		VkDescriptorPoolCreateInfo m_descriptor_pool_create_info;
		std::vector<VkDescriptorPool> m_descriptor_pools;
		std::vector<std::vector<VkDescriptorSet>> m_descriptor_sets; // first array is versions second array is sets
		std::vector<VkImageView> m_image_views; // stores image views for textures.
		std::vector<VkSampler> m_image_samplers; // store sampler for textures.
	};

} /* gfx */