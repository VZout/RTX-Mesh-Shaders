/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <string>
#include <cassert>
#include <mutex>

namespace util
{

	struct Progress
	{
        Progress()
        {}

        Progress(unsigned int max) : m_max(max)
        {}

        ~Progress()
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            delete m_child;
        }

		unsigned int m_progress = 0;
        unsigned int m_max = 0;
		std::string m_action = "Unknown";
        Progress* m_child = nullptr;

        std::mutex m_mutex;

        void SetMax(unsigned int max)
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            m_max = max;
        }

        void Increment(std::string const & action)
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            if (m_child)
            {
                m_child->Increment(action);
            }
            else
            {
                m_progress++;
				m_action = action;
            }
        }

        std::string GetAction()
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            return m_action;
        }

        float GetFraction()
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            return (static_cast<float>(m_progress) - 1.f) / static_cast<float>(m_max);
        }

        void MakeChild(unsigned int max)
        {
            const std::lock_guard<std::mutex> lock(m_mutex);

            if (m_child)
            {
                m_child->MakeChild(max);
            }
            else
            {
                m_child = new util::Progress(max);
            }
        }

        void PopChild()
        {
            const std::lock_guard<std::mutex> lock(m_mutex);

            if (m_child->m_child)
            {
                m_child->PopChild();
            }
            else
            {
                delete m_child;
                m_child = nullptr;
            }
        }

		void Lock()
		{
			m_mutex.lock();
		}

		void Unlock()
		{
			m_mutex.unlock();
		}

		Progress* GetChild()
		{
			assert(m_child);
			return m_child;

		}

		bool HasChild()
        {
            return m_child;
        }

	};

} /* util */

#define PROGRESS(progress, action) \
progress.Increment(action);

#define SET_NUM_TASKS(progress, num) \
progress.SetMax(num);

#define MAKE_CHILD_PROGRESS(progress, num) \
progress.MakeChild(num);

#define POP_CHILD_PROGRESS(progress) \
progress.PopChild();
