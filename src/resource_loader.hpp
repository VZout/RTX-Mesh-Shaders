/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

template<typename T>
class ResourceLoader
{
protected:
	using AnonResource = std::unique_ptr<T>;

public:

	explicit ResourceLoader(std::vector<std::string> const & supported_formats)
			: m_supported_formats(supported_formats)
	{

	}

	virtual ~ResourceLoader()
	{
		for (auto& data : m_loaded_resources)
		{
			data.reset();
		}
		m_loaded_resources.clear();
	}

	T* Load(std::string const & path)
	{
		// TODO: Check file extension
		auto data = LoadFromDisc(path);

		m_loaded_resources.push_back(std::move(data));

		return m_loaded_resources.back().get();
	}

	bool IsSupportedExtension(std::string const & ext)
	{
		std::string lc_ext = ext;
		std::transform(lc_ext.begin(), lc_ext.end(), lc_ext.begin(), ::tolower);
		for (auto supported_extension : m_supported_formats)
		{
			std::transform(supported_extension.begin(), supported_extension.end(), supported_extension.begin(), ::tolower);

			if (supported_extension == ext)
			{
				return true;
			}
		}

		return false;
	}

protected:
	virtual AnonResource LoadFromDisc(std::string const & path) = 0;

	std::vector<std::string> m_supported_formats;
	std::vector<AnonResource> m_loaded_resources;
};
