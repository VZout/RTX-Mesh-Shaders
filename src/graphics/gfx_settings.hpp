#pragma once

#include <vector>

namespace gfx::settings
{
	static const bool enable_validation_layers = true;
	static const std::vector<const char*> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};
}