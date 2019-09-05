/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

class Application;

namespace gfx
{

	class Context;
	class CommandQueue;
	class RenderWindow;
	class Shader;

} /* gfx */

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	void Init(Application* app);
	void Render();

private:
	gfx::Context* m_context;
	gfx::CommandQueue* m_direct_queue;
	gfx::RenderWindow* m_render_window;

	// Temporary
	gfx::Shader* m_vs;
	gfx::Shader* m_ps;
};