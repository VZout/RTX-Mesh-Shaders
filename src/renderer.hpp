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