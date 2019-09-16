/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "stb_image_loader.hpp"

#include <stb_image.h>

#include "util/log.hpp"
#include "resource_structs.hpp"

STBImageLoader::STBImageLoader()
	: ResourceLoader(std::vector<std::string>{".png", "jpg"})
{

}

STBImageLoader::AnonResource STBImageLoader::LoadFromDisc(std::string const & path)
{
	auto texture = std::make_unique<TextureData>();

	int width = 0, height = 0, channels = 0;
	unsigned char* x = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!x || width <= 0 || height <=0 || channels <= 0)
	{
		LOGC("STB Failed to load texture.");
	}

	texture->m_pixels = std::vector<unsigned char>(x, (x + width * height * 4)); // TODO: Unhardcode the 4. This will fuck me with different formats.
	texture->m_width = static_cast<std::uint32_t>(width);
	texture->m_height = static_cast<std::uint32_t>(height);
	texture->m_channels = static_cast<std::uint32_t>(channels);

	return texture;
}
