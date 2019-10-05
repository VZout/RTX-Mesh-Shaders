/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <mutex>
#include <vector>

#include "util/log.hpp"

#define REGISTER(type, registry) decltype(type) type = registry::Get().Register

using RegistryHandle = std::uint64_t;

namespace internal
{
	template<typename C, typename TO, typename TD>
	class Registry
	{
	protected:
		Registry() : m_requested_reload() { }

	public:
		virtual ~Registry() = default;

		void Clean()
		{
			m_descriptions.clear();
			m_objects.clear();
			m_requested_reload.clear();
		}

		// Static find.
		static inline TO* SFind(RegistryHandle handle)
		{
			return C::Get().Find(handle);
		}

		RegistryHandle Register(TD desc)
		{
			m_descriptions.push_back(desc);

			return m_descriptions.size() - 1;
		}

		inline TO* Find(RegistryHandle handle)
		{
			if (handle >= m_objects.size())
			{
				LOGE("Failed to find registry object related to registry handle.");
				return nullptr;
			}

			return m_objects[handle];
		}

		static C& Get()
		{
			static C instance = {};
			return instance;
		}

		inline void Lock()
		{
			m_mutex.lock();
		}

		inline void Unlock()
		{
			m_mutex.unlock();
		}

		inline void ClearReloadRequests()
		{
			m_requested_reload.clear();
		}

		inline std::vector<RegistryHandle> const & GetReloadRequests()
		{
			return m_requested_reload;
		}

		inline void RequestReload(RegistryHandle handle)
		{
			Lock();
			m_requested_reload.push_back(handle);
			Unlock();
		}

		inline std::vector<TD> const & GetDescriptions() const
		{
			return m_descriptions;
		}

		inline std::vector<TO*>& GetObjects()
		{
			return m_objects;
		}

	protected:
		std::vector<TD> m_descriptions;
		std::vector<TO*> m_objects;
		std::vector<RegistryHandle> m_requested_reload;
		std::mutex m_mutex;
	};

} /* internal */
