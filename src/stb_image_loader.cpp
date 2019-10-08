/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "stb_image_loader.hpp"

#include <stb_image.h>

#include "util/log.hpp"
#include "graphics/gfx_enums.hpp"
#include "resource_structs.hpp"

STBImageLoader::STBImageLoader()
	: ResourceLoader(std::vector<std::string>{"png", "jpg" })
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
	//channels = 4; //TODO: forcing a alpha component to make my job easier.

	auto data_size =  width * height * 4;
	texture->m_pixels = malloc(data_size); // TODO: Destroy this
	memcpy(texture->m_pixels, x, data_size);
	texture->m_width = static_cast<std::uint32_t>(width);
	texture->m_height = static_cast<std::uint32_t>(height);
	texture->m_channels = static_cast<std::uint32_t>(channels);
	texture->m_is_hdr = false;

	return texture;
}

STBHDRImageLoader::STBHDRImageLoader()
		: ResourceLoader(std::vector<std::string>{ "hdr"})
{

}

STBHDRImageLoader::AnonResource STBHDRImageLoader::LoadFromDisc(std::string const & path)
{
	auto texture = std::make_unique<TextureData>();

	int width = 0, height = 0, channels = 0;
	float* x = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!x || width <= 0 || height <=0 || channels <= 0)
	{
		LOGC("STB Failed to load texture.");
	}

	auto data_size = width * height * gfx::enums::BytesPerPixel(VK_FORMAT_R32G32B32A32_SFLOAT); // TODO: 4 but only 3 components.
	texture->m_pixels = malloc(data_size); // TODO: Destroy this
	memcpy(texture->m_pixels, x, data_size);
	texture->m_width = static_cast<std::uint32_t>(width);
	texture->m_height = static_cast<std::uint32_t>(height);
	texture->m_channels = static_cast<std::uint32_t>(channels);
	texture->m_is_hdr = true;

	return texture;
}
