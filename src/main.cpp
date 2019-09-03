#include <iostream>

#include "scene_graph.hpp"
#include "window.hpp"

class Demo : public Application
{
public:
	Demo() : Application("Mesh Shaders Demo")
	{

	}

protected:
	void Init() final
	{
		std::cout << "Hello World\n";

		auto sg = new SceneGraph();

		auto node = sg->CreateNode();
		sg->PromoteNode<MeshComponent>(node);

		delete sg;
	}

	void Loop() final
	{
		// Do stuff
	}
};


ENTRY(Demo, 1280, 720)