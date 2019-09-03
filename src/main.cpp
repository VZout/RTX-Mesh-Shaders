#include <iostream>

#include "scene_graph.hpp"

int main()
{
	std::cout << "Hello World\n";

	auto sg = new SceneGraph();

	auto node = sg->CreateNode();
	sg->PromoteNode<MeshComponent>(node);

	delete sg;

	return 0;
}
