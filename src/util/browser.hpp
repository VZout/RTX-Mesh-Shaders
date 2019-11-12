/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#ifdef _WIN32
#include <shellapi.h>
#endif

namespace util
{

	// Open a URL in the default browser.
	static inline void OpenURL(std::string const & url)
	{
#ifdef _WIN32
		ShellExecuteA(0, 0, url.c_str(), 0, 0 , SW_SHOW );
#endif
	}

} /* util */
