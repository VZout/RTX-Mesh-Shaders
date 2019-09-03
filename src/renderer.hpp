#pragma once

#include "graphics/context.hpp"

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	void Init();
	void Render();

private:
	gfx::Context* m_context;
};