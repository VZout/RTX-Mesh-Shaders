/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "texture_pool.hpp"

TexturePool::TexturePool()
	: m_next_id(0)
{

}

std::uint32_t TexturePool::Load(std::string const& path, bool srgb)
{
	auto extension = path.substr(path.find_last_of('.') + 1);
	auto new_id = m_next_id;

	for (auto& loader : m_registered_loaders)
	{
		loader->IsSupportedExtension(extension);
		auto texture_data = loader->Load(path);

		Load_Impl(*texture_data, new_id, srgb);

		break;
	}

	m_next_id++;
	return new_id;
}

std::uint32_t TexturePool::Load(TextureData const & data, bool srgb)
{
	auto new_id = m_next_id;
	Load_Impl(data, new_id, srgb);

	m_next_id++;
	return new_id;
}