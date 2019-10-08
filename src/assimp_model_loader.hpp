/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include "resource_loader.hpp"

#include "resource_structs.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class AssimpModelLoader : public ResourceLoader<ModelData>
{
public:
	AssimpModelLoader();
	~AssimpModelLoader() final = default;

	AnonResource LoadFromDisc(std::string const & path) final;

	void LoadEmbeddedTextures(ModelData* model, const aiScene* scene);
	void LoadMaterials(ModelData* model, const aiScene* scene);
	void LoadMeshes(ModelData* model, const aiScene* scene, aiNode* node);
};