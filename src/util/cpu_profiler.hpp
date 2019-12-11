/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#define PROFILER_GRAPHING
#define PROFILER_GRAPHING_MAX_SAMPLES 500

#include "log.hpp"

#include <chrono>
#include <unordered_map>
#include <string>

namespace util
{

	class CPUProfilerSystem
	{
	private:
		CPUProfilerSystem() {}

		~CPUProfilerSystem()
		{
			for (auto const & it : m_scopes)
			{
				double nanoseconds = it.second.m_total / 1000000.0;
				double average = nanoseconds / it.second.m_times;

				LOG("ScopeTimer '{}' called {} times, total time: {} ms (avg: {})", it.first, it.second.m_times, nanoseconds, average);
			}
		}

	public:
		struct ScopeBlock
		{
			using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

			TimePoint m_start;
			std::int64_t m_total = 0;
			std::int64_t m_last = 0;
			std::int64_t m_times = 0;
			bool m_in_use = false;

#ifdef PROFILER_GRAPHING
			std::vector<float> m_samples;
#endif
		};

		CPUProfilerSystem(CPUProfilerSystem const&) = delete;
        void operator=(CPUProfilerSystem const&) = delete;

		static CPUProfilerSystem& Get()
		{
			static CPUProfilerSystem instance = {};
			return instance;
		}

		void AddTime(std::string name, std::int64_t ns)
		{
			if (auto it = m_scopes.find(name); it != m_scopes.end())
			{
				it->second.m_total += ns;
				it->second.m_last = ns;
				it->second.m_times++;
				it->second.m_in_use = false;
#ifdef PROFILER_GRAPHING
				auto& samples = it->second.m_samples;
				if (samples.size() > PROFILER_GRAPHING_MAX_SAMPLES) //Max seconds to show
				{
					for (size_t i = 1; i < samples.size(); i++)
					{
						samples[i - 1] = samples[i];
					}
					samples[samples.size() - 1] = ns;
				}
				else
				{
					samples.push_back(ns);
				}
#endif
			}
			else
			{
				m_scopes[name] = ScopeBlock();
			}
		}

		bool Aquire(std::string name)
		{
			if (auto it = m_scopes.find(name); it != m_scopes.end())
			{
				if (it->second.m_in_use)
				{
					return false;
				}

				it->second.m_in_use = true;
				return true;
			}
			else
			{
				ScopeBlock new_scope;
				new_scope.m_in_use = true;
				m_scopes[name] = new_scope;
				return true;
			}

			LOGW("Profiler tried to aquire a scope block that doesn't exist");
			return false;
		}

		std::unordered_map<std::string, ScopeBlock> const & GetScopes() const
		{
			return m_scopes;
		}

	private:
		std::unordered_map<std::string, ScopeBlock> m_scopes;
	};

	class OneTime
	{
	public:
		OneTime(std::string name)
			: m_aquired(false), m_name(name)
        {
			auto& ps = CPUProfilerSystem::Get();

            if (ps.Aquire(name))
			{
                m_start = std::chrono::high_resolution_clock::now();
				m_aquired = true;
            }
            else
            {
				m_aquired = false;
            }
        }

        ~OneTime()
        {
			auto& ps = CPUProfilerSystem::Get();

            if (m_aquired)
			{
                std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
                ps.AddTime(m_name, (end - m_start).count());
            }
        }

	private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
        bool m_aquired;
		std::string m_name;
	};

} /* util */

#define TIME_THIS_SCOPE(name) \
    util::OneTime st_one_time_##name(#name)
