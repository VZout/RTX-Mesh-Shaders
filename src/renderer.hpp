/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "graphics/context.hpp"
#include "graphics/command_queue.hpp"

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	void Init();
	void Render();

private:
	gfx::Context* m_context;
	gfx::CommandQueue* m_direct_queue;
};