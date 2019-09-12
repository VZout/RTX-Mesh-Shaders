/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "model_loader.hpp"

class TinyGLTFModelLoader : public ModelLoader
{
public:
	TinyGLTFModelLoader();
	~TinyGLTFModelLoader() final = default;

	ModelData* LoadFromDisc(std::string const & path) final;
};