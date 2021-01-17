
#include "stdafx.h"
#include "AssImp.h"
#include "Model.h"
#include "WinUtil.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class AssImpDelayed : public WinUtils::DelayedLib
{
public:
	AssImpDelayed() : WinUtils::DelayedLib("assimp-vc142-mt.dll") {}

	typedef const aiScene* (*aiImportFileFP)(const char* pFile, unsigned int pFlags);
	typedef const char* (*aiGetErrorStringFP)();
	typedef void(*aiReleaseImportFP)(const aiScene* pScene);
	typedef aiReturn(*aiGetMaterialColorFP)(const aiMaterial* pMat, const char* pKey, unsigned int type, unsigned int index, aiColor4D* pOut);

	aiImportFileFP aiImportFile = nullptr;
	aiGetErrorStringFP aiGetErrorString = nullptr;
	aiReleaseImportFP aiReleaseImport = nullptr;
	aiGetMaterialColorFP aiGetMaterialColor = nullptr;

protected:
	void Init() override
	{
		LoadFunction(aiImportFile, "aiImportFile");
		LoadFunction(aiGetErrorString, "aiGetErrorString");
		LoadFunction(aiReleaseImport, "aiReleaseImport");
		LoadFunction(aiGetMaterialColor, "aiGetMaterialColor");

	}
} assImpDelayed;

const aiScene* AssImp::ImportFile(const std::string& modelpath, bool correctDodgyModel)
{
	assImpDelayed.Setup();

	int flags = 0;
	if (correctDodgyModel)
		flags = aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;	/*aiProcess_FlipWindingOrder*/
	const aiScene* scene = assImpDelayed.aiImportFile(modelpath.c_str(), flags);

	if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
		return nullptr;
	else
		return scene;
}

class SceneProcessor
{
public:
	SceneProcessor(const aiScene& scene, Model& model, const std::vector<Attribs::Attrib>& attribs)
		: scene(scene), attribs(attribs), useIndices(model.useIndices), vertexStride(model.vertexStride), vertices(model.vertices), indices(model.indices)
	{
		hasher.dataSize = vertexStride / sizeof(int);
	}
	void ProcessNode(const aiNode& node, aiMatrix4x4 transformation);

protected:
	void ProcessMesh(const aiMesh& mesh, const aiMatrix4x4 &transformation);
	int FindVertex(int hash, char* data);

	struct Hasher
	{
		int operator()(void* data) const
		{
			int* iD = (int*)data;
			const int prime1 = 7;
			const int prime2 = 31;
			int result = prime1;
			for (size_t i = 0; i < dataSize; ++i) {
				result = iD[i] + (result * prime2);
			}
			return result;
		}
		size_t dataSize;
	} hasher;

private:
	const aiScene& scene;
	const std::vector<Attribs::Attrib>& attribs;
	bool useIndices;
	uint32_t vertexStride;

	std::vector<char>& vertices;
	std::vector<uint32_t>& indices;

	std::unordered_multimap<int, int> vertexChecker;
};

void AssImp::ProcessScene(const aiScene& scene, Model& model, const std::vector<Attribs::Attrib>& attribs, const std::array<double, 3>& translation)
{
	SceneProcessor sp(scene, model, attribs);

	aiMatrix4x4 transformation;
	// Apply translation (normally blank)
	aiMatrix4x4t<double> dummy;
	aiVector3t<double> assTrans(translation[0], translation[1], translation[2]);
	transformation *= aiMatrix4x4t<double>::Translation(assTrans, dummy);

	// Process ASSIMP's root node recursively
	sp.ProcessNode(*scene.mRootNode, transformation);
}

std::string AssImp::GetErrorString()
{
	assImpDelayed.Setup();
	return assImpDelayed.aiGetErrorString();
}

void AssImp::Tidy(const aiScene* scene)
{
	assImpDelayed.Setup();
	assImpDelayed.aiReleaseImport(scene);
}

void ProcessVertex(char*& data, const std::vector<Attribs::Attrib>& attribs, aiVector3D vertex, const aiMatrix4x4& transformation, aiVector3D* pNormal, aiVector3D* textureCoords, aiVector3D* pTangents, aiVector3D* pBiTangents, const aiColor4D& colour)
{
	auto AddData = [&data](auto val)
	{
		*((decltype(val)*)data) = val;
		data += sizeof(val);
	};

	for (auto& attrib : attribs)
	{
		switch (attrib.type)
		{
		case Attribs::Type::Position:
			vertex *= transformation;
			AddData(vertex.x);
			AddData(-vertex.y);
			AddData(vertex.z);
			break;
		case Attribs::Type::Colour:
			AddData(colour[0]);
			AddData(colour[1]);
			AddData(colour[2]);
			break;
		case Attribs::Type::Normal:
			if (pNormal)
			{
				AddData(pNormal->x);
				AddData(-pNormal->y);
				AddData(pNormal->z);
			}
			else
			{
				AddData(0.0f);
				AddData(0.0f);
				AddData(0.0f);
			}
			break;
		case Attribs::Type::Texture:
			if (textureCoords)
			{
				AddData(textureCoords->x);
				AddData(textureCoords->y);
			}
			else
			{
				AddData(0.0f);
				AddData(0.0f);
			}
			break;
		case Attribs::Type::Tangent:
			if (pTangents)
			{
				AddData(pTangents->x);
				AddData(-pTangents->y);
				AddData(pTangents->z);
			}
			else
			{
				AddData(0.0f);
				AddData(0.0f);
				AddData(0.0f);
			}
			break;
		case Attribs::Type::BiTangent:
			if (pBiTangents)
			{
				AddData(pBiTangents->x);
				AddData(-pBiTangents->y);
				AddData(pBiTangents->z);
			}
			else
			{
				AddData(0.0f);
				AddData(0.0f);
				AddData(0.0f);
			}
			break;
		}
	}
}

int SceneProcessor::FindVertex(int hash, char* data)
{
	auto range = vertexChecker.equal_range(hash);
	for (auto it = range.first; it != range.second; it++)
	{
		if (memcmp(&vertices[(*it).second], data, vertexStride) == 0)
			return (*it).second / vertexStride;	// Found
	}
	return -1;
}

void SceneProcessor::ProcessMesh(const aiMesh& mesh, const aiMatrix4x4 &transformation)
{
	aiColor4D colour;
	assImpDelayed.aiGetMaterialColor(scene.mMaterials[mesh.mMaterialIndex], AI_MATKEY_COLOR_DIFFUSE, &colour);

	auto curSize = vertices.size();
	vertices.resize(curSize + mesh.mNumVertices * vertexStride);
	char* data = &vertices[curSize];

	// Walk through each of the mesh's vertices
	for (unsigned int i = 0; i < mesh.mNumVertices; i++)
	{
		ProcessVertex(data, attribs, mesh.mVertices[i], transformation, (mesh.mNormals ? &mesh.mNormals[i] : nullptr), (mesh.mTextureCoords[0] ? &mesh.mTextureCoords[0][i] : nullptr),
			(mesh.HasTangentsAndBitangents() ? &mesh.mTangents[i] : nullptr), (mesh.HasTangentsAndBitangents() ? &mesh.mBitangents[i] : nullptr), colour);

		if (useIndices)
		{
			if (mesh.mNumVertices == mesh.mNumFaces * 3)
			{	// Assume all points provided so optimse
				char* vData = data - vertexStride;
				int hash = hasher(vData);
				int pos = FindVertex(hash, vData);
				if (pos >= 0)
				{	// Vertex found - reuse
					indices.push_back(pos);
					data = vData;	// Skip duplicate vertex
				}
				else
				{	// New vertex
					int vertexOffset = (int)(vData - vertices.data());
					vertexChecker.emplace(hash, vertexOffset);
					indices.push_back(vertexOffset / vertexStride);
				}
			}
		}
	}

	if (useIndices)
	{
		if (mesh.mNumVertices == mesh.mNumFaces * 3)
		{
			// Erase unused vertices
			int vertexOffset = (int)(data - vertices.data());
			vertices.erase(vertices.begin() + vertexOffset, vertices.end());
		}
		else
		{	// Read indicies from model
			uint32_t indexBase = static_cast<uint32_t>(indices.size());
			for (unsigned int j = 0; j < mesh.mNumFaces; j++)
			{
				const aiFace& Face = mesh.mFaces[j];
				if (Face.mNumIndices != 3)
					continue;
				indices.push_back(indexBase + Face.mIndices[0]);
				indices.push_back(indexBase + Face.mIndices[1]);
				indices.push_back(indexBase + Face.mIndices[2]);
			}
		}
	}

	//TODO use model materials to define textures?
	/*
	if (mesh.mMaterialIndex > 0)
	{
		aiMaterial* material = scene.mMaterials[mesh.mMaterialIndex];
	}
	*/
}

void SceneProcessor::ProcessNode(const aiNode& node, aiMatrix4x4 transformation)
{
	transformation *= node.mTransformation;
	// Process each mesh located at the current node
	for (unsigned int i = 0; i < node.mNumMeshes; i++)
	{
		// The node object only contains indices to index the actual objects in the scene. 
		// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh& mesh = *scene.mMeshes[node.mMeshes[i]];
		ProcessMesh(mesh, transformation);
	}
	// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node.mNumChildren; i++)
	{
		ProcessNode(*node.mChildren[i], transformation);
	}
}
