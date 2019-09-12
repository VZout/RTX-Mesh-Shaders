/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "tinygltf_model_loader.hpp"

#include <algorithm>

ModelLoader::ModelLoader(std::vector<std::string> const & supported_formats)
	: m_supported_formats(supported_formats)
{

}

ModelData* ModelLoader::Load(std::string const & path)
{
	// TODO: Check file extension
	auto data =  LoadFromDisc(path);

	m_loaded_models.push_back(data);

	return data;
}

void ModelLoader::Remove(ModelData* model_data)
{
	m_loaded_models.erase(std::remove(m_loaded_models.begin(), m_loaded_models.end(), model_data), m_loaded_models.end());
	delete model_data;
}

ModelLoader::~ModelLoader()
{
	for (auto& data : m_loaded_models)
	{
		delete data;
	}
	m_loaded_models.clear();
}
