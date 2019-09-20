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

void gfx::VkTexturePool::Load_Impl(TextureData const & data, std::uint32_t id, bool srgb)
{
	auto desc = StagingTexture::Desc();
	desc.m_width = data.m_width;
	desc.m_height = data.m_height;
	desc.m_format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	auto texture = new StagingTexture(m_context, desc, const_cast<unsigned char*>(data.m_pixels.data()));
	m_queued_for_staging_textures.insert(std::make_pair(id, texture));
}

void gfx::VkTexturePool::Stage(gfx::CommandList* command_list)
{
	for (auto& texture : m_queued_for_staging_textures)
	{
		command_list->TransitionTexture(texture.second, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		command_list->StageTexture(texture.second);
		command_list->TransitionTexture(texture.second, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
