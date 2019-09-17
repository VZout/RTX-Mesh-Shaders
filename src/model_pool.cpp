/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "model_pool.hpp"

ModelPool::ModelPool()
	: m_next_id(0)
{

}

ModelData* ModelPool::GetRawData(ModelHandle handle)
{
	if (auto it = m_loaded_data.find(handle); it != m_loaded_data.end())
	{
		return it->second;
	}

	LOGE("Failed to find raw data from handle");
	return nullptr;
}