/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "vk_texture_pool.hpp"

#include "command_list.hpp"
#include "gpu_buffers.hpp"
#include "descriptor_heap.hpp"
#include "../util/log.hpp"

gfx::VkTexturePool::VkTexturePool(gfx::Context* context)
	: m_context(context)
{
}

gfx::VkTexturePool::~VkTexturePool()
{
	for (auto& texture : m_staged_textures)
	{
		delete texture.second;
	}
	m_staged_textures.clear();

	for (auto& texture : m_queued_for_staging_textures)
	{
		delete texture.second;
	}
	m_queued_for_staging_textures.clear();
}

void gfx::VkTexturePool::Load_Impl(TextureData const & data, std::uint32_t id, bool mipmap, bool srgb)
{
	auto desc = StagingTexture::Desc();
	desc.m_width = data.m_width;
	desc.m_height = data.m_height;
	desc.m_channels = data.m_channels;
	desc.m_mip_levels = mipmap ? static_cast<std::uint32_t>(std::floor(std::log2(std::max(desc.m_width, desc.m_height)))) + 1 : 1;
	desc.m_mip_levels = std::clamp(desc.m_mip_levels, 1u, 16u);

	if (data.m_is_hdr && srgb)
	{
		LOGW("A texture is specified as HDR and SRGB. This is not supported. Using the HDR format instead.");
	}

	if (data.m_is_hdr)
	{
		desc.m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	else
	{
		desc.m_format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	}

	// TODO: memory pool
	auto texture = new StagingTexture(m_context, std::nullopt, desc, data.m_pixels);
	m_queued_for_staging_textures.insert(std::make_pair(id, texture));
}

void gfx::VkTexturePool::Stage(gfx::CommandList* command_list)
{
	for (auto& texture : m_queued_for_staging_textures)
	{
		command_list->TransitionTexture(texture.second, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		command_list->StageTexture(texture.second);

		// Generate mipmaps or transition it to shader read only.
		if (texture.second->HasMipMaps())
		{
			command_list->GenerateMipMap(texture.second);
		}
		else
		{
			command_list->TransitionTexture(texture.second, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		m_staged_textures.insert(texture);
		m_queued_for_release_staging_resources_textures.push_back(texture.first);
	}

	m_queued_for_staging_textures.clear();
}

void gfx::VkTexturePool::PostStage()
{
	for (auto idx : m_queued_for_release_staging_resources_textures)
	{
		m_staged_textures[idx]->FreeStagingResources();
	}
	m_queued_for_release_staging_resources_textures.clear();
}

std::vector<gfx::StagingTexture*> gfx::VkTexturePool::GetTextures(std::vector<std::uint32_t> texture_handles)
{
	std::vector<gfx::StagingTexture*> textures;

	for (auto handle : texture_handles)
	{
		bool found = false;

		if (auto it = m_staged_textures.find(handle); it != m_staged_textures.end())
		{
			textures.push_back(it->second);
			found = true;
		}

		if (auto it = m_queued_for_staging_textures.find(handle); it != m_queued_for_staging_textures.end())
		{
			textures.push_back(it->second);
			found = true;
		}

		if (!found)
		{
			LOGE("Failed to find texture inside texture pool.");
		}
	}

	return textures;
}

std::vector<gfx::StagingTexture*> gfx::VkTexturePool::GetAllTexturesPadded(std::uint32_t num)
{
	std::vector<gfx::StagingTexture*> all_textures;
	for (auto& texture : m_staged_textures)
	{
		all_textures.push_back(texture.second);
	}
	all_textures.resize(num, m_staged_textures.begin()->second);

	return all_textures;
}
