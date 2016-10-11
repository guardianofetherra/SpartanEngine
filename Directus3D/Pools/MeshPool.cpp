/*
Copyright(c) 2016 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =======================
#include "MeshPool.h"
#include "../Core/GUIDGenerator.h"
#include "../IO/Serializer.h"
#include "../Core/GameObject.h"
#include "../Components/MeshFilter.h"
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
//==================================

//= NAMESPACES ================
using namespace std;
using namespace Directus::Math;
//=============================

MeshPool::MeshPool()
{
	GenerateDefaultMeshes();
}

MeshPool::~MeshPool()
{
	Clear();
}

/*------------------------------------------------------------------------------
									[MISC]
------------------------------------------------------------------------------*/
void MeshPool::Clear()
{
	m_meshes.clear();
	m_meshes.shrink_to_fit();
}

// Adds a mesh to the pool directly from memory
shared_ptr<Mesh> MeshPool::Add(const string& name, const string& rootGameObjectID, const vector<VertexPositionTextureNormalTangent>& vertices, const vector<unsigned int>& indices)
{
	// construct the mesh
	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->SetName(name);
	mesh->SetRootGameObjectID(rootGameObjectID);
	mesh->SetVertices(vertices);
	mesh->SetIndices(indices);
	mesh->Update();

	// add it to the pool
	m_meshes.push_back(mesh);

	// return it
	return m_meshes.back();
}

// Adds multiple meshes to the pool by reading them from files
void MeshPool::Add(const vector<string>& filePaths)
{
	for (const string& filePath : filePaths)
	{
		// Make sure the path is valid
		if (!FileSystem::FileExists(filePath))
			continue;

		// Make sure it's actually a mesh file
		if (FileSystem::GetExtensionFromPath(filePath) != MESH_EXTENSION)
			continue;

		shared_ptr<Mesh> mesh = make_shared<Mesh>();
		if (mesh->LoadFromFile(filePath))
			m_meshes.push_back(mesh);
	}
}

shared_ptr<Mesh> MeshPool::GetMeshByID(const string& ID)
{
	if (ID == MESH_DEFAULT_CUBE_ID)
		return m_defaultCube;

	if (ID == MESH_DEFAULT_QUAD_ID)
		return m_defaultQuad;

	for (shared_ptr<Mesh> mesh : m_meshes)
		if (mesh->GetID() == ID)
			return mesh;

	return nullptr;
}

shared_ptr<Mesh> MeshPool::GetMeshByPath(const string& path)
{
	for (shared_ptr<Mesh> mesh : m_meshes)
		if (mesh->GetFilePath() == path)
			return mesh;

	return nullptr;
}

vector<string> MeshPool::GetAllMeshFilePaths()
{
	vector<string> paths;

	for (shared_ptr<Mesh> mesh : m_meshes)
		paths.push_back(mesh->GetFilePath());

	return paths;
}

// Returns the meshes tha belong to the same model
vector<shared_ptr<Mesh>> MeshPool::GetModelMeshesByModelName(const string& rootGameObjectID)
{
	vector<shared_ptr<Mesh>> modelMeshes;

	for (shared_ptr<Mesh> mesh : m_meshes)
		if (mesh->GetRootGameObjectID() == rootGameObjectID)
			modelMeshes.push_back(mesh);

	return modelMeshes;
}

int MeshPool::GetMeshCount()
{
	return (int)m_meshes.size();
}

shared_ptr<Mesh> MeshPool::GetDefaultCube()
{
	return m_defaultCube;
}

shared_ptr<Mesh> MeshPool::GetDefaultQuad()
{
	return m_defaultQuad;
}

/*------------------------------------------------------------------------------
							[MESH PROCESSING]
------------------------------------------------------------------------------*/
// Returns a value that can be used (by multiplying against the original scale)
// to normalize the scale of a transform
float MeshPool::GetNormalizedModelScaleByRootGameObjectID(const string& rootGameObjectID)
{
	// get all the meshes related to this model
	vector<shared_ptr<Mesh>> modelMeshes = GetModelMeshesByModelName(rootGameObjectID);

	// find the mesh with the largest bounding box
	shared_ptr<Mesh> largestBoundingBoxMesh = GetLargestBoundingBox(modelMeshes);

	if (!largestBoundingBoxMesh)
		return 1.0f;

	// calculate the scale
	Vector3 boundingBox = largestBoundingBoxMesh->GetBoundingBox();
	float scaleOffset = boundingBox.Length();

	return 1.0f / scaleOffset;
}

void MeshPool::SetModelScale(const string& rootGameObjectID, float scale)
{
	// get all the meshes related to this model
	vector<shared_ptr<Mesh>> modelMeshes = GetModelMeshesByModelName(rootGameObjectID);

	for (auto i = 0; i < modelMeshes.size(); i++)
		modelMeshes[i]->Scale(scale);
}

void MeshPool::NormalizeModelScale(GameObject* rootGameObject)
{
	if (!rootGameObject)
		return;

	float normalizedScale = GetNormalizedModelScaleByRootGameObjectID(rootGameObject->GetID());
	SetModelScale(rootGameObject->GetID(), normalizedScale);
}

// Returns the largest bounding box in an vector of meshes
shared_ptr<Mesh> MeshPool::GetLargestBoundingBox(const vector<shared_ptr<Mesh>>& meshes)
{
	if (meshes.empty())
		return nullptr;

	Vector3 largestBoundingBox = Vector3::Zero;
	shared_ptr<Mesh> largestBoundingBoxMesh = meshes.front();

	for (shared_ptr<Mesh> mesh : meshes)
	{
		if (!mesh)
			continue;

		Vector3 boundingBox = mesh->GetBoundingBox();
		if (boundingBox.Volume() > largestBoundingBox.Volume())
		{
			largestBoundingBox = boundingBox;
			largestBoundingBoxMesh = mesh;
		}
	}

	return largestBoundingBoxMesh;
}

void MeshPool::GenerateDefaultMeshes()
{
	vector<VertexPositionTextureNormalTangent> vertices;
	vector<unsigned int> indices;

	CreateCube(vertices, indices);

	// construct the mesh
	m_defaultCube = make_shared<Mesh>();
	m_defaultCube->SetID(MESH_DEFAULT_CUBE_ID);
	m_defaultCube->SetName("Cube");
	m_defaultCube->SetVertices(vertices);
	m_defaultCube->SetIndices(indices);	
	m_defaultCube->Update();

	vertices.clear();
	vertices.shrink_to_fit();

	CreateQuad(vertices, indices);

	m_defaultQuad = make_shared<Mesh>();
	m_defaultQuad->SetID(MESH_DEFAULT_QUAD_ID);
	m_defaultQuad->SetName("Quad");
	m_defaultQuad->SetVertices(vertices);
	m_defaultQuad->SetIndices(indices);
	m_defaultQuad->Update();

	vertices.clear();
	vertices.shrink_to_fit();
}

void MeshPool::CreateCube(vector<VertexPositionTextureNormalTangent>& vertices, vector<unsigned int>& indices)
{
	// front
	vertices.push_back({ Vector3(-0.5f, -0.5f, -0.5f), Vector2(0, 1), Vector3(0, 0, -1), Vector3(0, 1, 0) }); // 0
	vertices.push_back({ Vector3(-0.5f, 0.5f, -0.5f), Vector2(0, 0), Vector3(0, 0, -1), Vector3(0, 1, 0) }); // 1
	vertices.push_back({ Vector3(0.5f, -0.5f, -0.5f), Vector2(1, 1), Vector3(0, 0, -1), Vector3(0, 1, 0) }); // 2
	vertices.push_back({ Vector3(0.5f, 0.5f, -0.5f), Vector2(1, 0), Vector3(0, 0, -1), Vector3(0, 1, 0) }); // 3

																											// bottom
	vertices.push_back({ Vector3(-0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector3(0, -1, 0), Vector3(1, 0, 0) }); // 4
	vertices.push_back({ Vector3(-0.5f, -0.5f, -0.5f), Vector2(0, 0), Vector3(0, -1, 0), Vector3(1, 0, 0) }); // 5
	vertices.push_back({ Vector3(0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector3(0, -1, 0), Vector3(1, 0, 0) }); // 6
	vertices.push_back({ Vector3(0.5f, -0.5f, -0.5f), Vector2(1, 0), Vector3(0, -1, 0), Vector3(1, 0, 0) }); // 7

																											 // back
	vertices.push_back({ Vector3(-0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector3(0, 0, 1), Vector3(0, 1, 0) }); // 8
	vertices.push_back({ Vector3(-0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector3(0, 0, 1), Vector3(0, 1, 0) }); // 9
	vertices.push_back({ Vector3(0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector3(0, 0, 1), Vector3(0, 1, 0) }); // 10
	vertices.push_back({ Vector3(0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector3(0, 0, 1), Vector3(0, 1, 0) }); // 11

																										  // top
	vertices.push_back({ Vector3(-0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 12
	vertices.push_back({ Vector3(-0.5f, 0.5f, -0.5f), Vector2(0, 1), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 13
	vertices.push_back({ Vector3(0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 14
	vertices.push_back({ Vector3(0.5f, 0.5f, -0.5f), Vector2(1, 1), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 15

																										   // left
	vertices.push_back({ Vector3(-0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector3(-1, 0, 0), Vector3(0, 1, 0) }); // 16
	vertices.push_back({ Vector3(-0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector3(-1, 0, 0), Vector3(0, 1, 0) }); // 17
	vertices.push_back({ Vector3(-0.5f, -0.5f, -0.5f), Vector2(1, 1), Vector3(-1, 0, 0), Vector3(0, 1, 0) }); // 18
	vertices.push_back({ Vector3(-0.5f, 0.5f, -0.5f), Vector2(1, 0), Vector3(-1, 0, 0), Vector3(0, 1, 0) }); // 19

																											 // right
	vertices.push_back({ Vector3(0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector3(1, 0, 0), Vector3(0, 1, 0) }); // 20
	vertices.push_back({ Vector3(0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector3(1, 0, 0), Vector3(0, 1, 0) }); // 21
	vertices.push_back({ Vector3(0.5f, -0.5f, -0.5f), Vector2(0, 1), Vector3(1, 0, 0), Vector3(0, 1, 0) }); // 22
	vertices.push_back({ Vector3(0.5f, 0.5f, -0.5f), Vector2(0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0) }); // 23

																										   // front
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(1);
	indices.push_back(3);

	// bottom
	indices.push_back(4);
	indices.push_back(5);
	indices.push_back(6);
	indices.push_back(6);
	indices.push_back(5);
	indices.push_back(7);

	// back
	indices.push_back(10);
	indices.push_back(9);
	indices.push_back(8);
	indices.push_back(11);
	indices.push_back(9);
	indices.push_back(10);

	// top
	indices.push_back(14);
	indices.push_back(13);
	indices.push_back(12);
	indices.push_back(15);
	indices.push_back(13);
	indices.push_back(14);

	// left
	indices.push_back(16);
	indices.push_back(17);
	indices.push_back(18);
	indices.push_back(18);
	indices.push_back(17);
	indices.push_back(19);

	// left
	indices.push_back(22);
	indices.push_back(21);
	indices.push_back(20);
	indices.push_back(23);
	indices.push_back(21);
	indices.push_back(22);
}

void MeshPool::CreateQuad(vector<VertexPositionTextureNormalTangent>& vertices, vector<unsigned int>& indices)
{
	vertices.push_back({ Vector3(-0.5f, 0.0f, 0.5f),Vector2(0, 0), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 0 top-left
	vertices.push_back({ Vector3(0.5f, 0.0f, 0.5f), Vector2(1, 0), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 1 top-right
	vertices.push_back({ Vector3(-0.5f, 0.0f, -0.5f), Vector2(0, 1), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 2 bottom-left
	vertices.push_back({ Vector3(0.5f, 0.0f, -0.5f),Vector2(1, 1), Vector3(0, 1, 0), Vector3(1, 0, 0) }); // 3 bottom-right

	indices.push_back(3);
	indices.push_back(2);
	indices.push_back(0);
	indices.push_back(3);
	indices.push_back(0);
	indices.push_back(1);
}