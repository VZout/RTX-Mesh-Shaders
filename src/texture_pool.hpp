/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>

#include "resource_loader.hpp"
#include "resource_structs.hpp"

namespace gfx
{
	class CommandList;
	class StagingTexture;
}

class TexturePool
{
public:
	TexturePool();
	virtual ~TexturePool() = default;

	std::uint32_t Load(std::string const & path, bool mipmap, bool srgb = false);
	std::uint32_t Load(TextureData const & data, bool mipmap, bool srgb = false);

	virtual void Stage(gfx::CommandList* command_list) = 0;
	virtual void PostStage() = 0;
	virtual std::vector<gfx::StagingTexture*> GetTextures(std::vector<std::uint32_t> texture_handles) = 0;

	template<typename T>
	static void RegisterLoader();

private:
	virtual void Load_Impl(TextureData const & data, std::uint32_t id, bool mipmap, bool srgb) = 0;

	std::uint32_t m_next_id;

	inline static std::vector<ResourceLoader<TextureData>*> m_registered_loaders = {};
};

template<typename T>
void TexturePool::RegisterLoader()
{
	m_registered_loaders.push_back(new T());
}