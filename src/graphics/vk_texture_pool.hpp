/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "../texture_pool.hpp"

#include <unordered_map>

namespace gfx
{

	class Context;
	class StagingTexture;
	class DescriptorHeap;

	class VkTexturePool : public TexturePool
	{
	public:
		explicit VkTexturePool(Context* context);
		~VkTexturePool() final;

		void Stage(gfx::CommandList* command_list) final;
		void PostStage() final;
		std::vector<gfx::StagingTexture*> GetTextures(std::vector<std::uint32_t> texture_handles) final;

	private:
		void Load_Impl(TextureData const & data, std::uint32_t id, bool mipmap, bool srgb) final;

		Context* m_context;

		std::unordered_map<std::uint32_t, StagingTexture*> m_queued_for_staging_textures;
		std::vector<std::uint32_t> m_queued_for_release_staging_resources_textures;
		std::unordered_map<std::uint32_t, StagingTexture*> m_staged_textures;
	};

} /* gfx */
