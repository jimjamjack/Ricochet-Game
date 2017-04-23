#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>
#include <vector>
#include <iostream>
using namespace std;

#include <glm/glm.hpp>

#include "ObjLoader.h"

// Include AssImp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

bool loadModel(const char* modelpath, std::vector<unsigned short> &indices, std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<glm::vec3> &normals) {

	// Use the Assimp importer
	Assimp::Importer objectImporter;

	// Read in the .obj file, joining identical vertices and sorting the file contents by type
	const aiScene* gameScene = objectImporter.ReadFile(modelpath, 0/*aiProcess_JoinIdenticalVertices | aiProcess_SortByPType*/);
	if (!gameScene) {
		cout << objectImporter.GetErrorString() << endl;
		return false;
	}
	const aiMesh* objectMesh = gameScene->mMeshes[0];
	unsigned int verticeCount = objectMesh->mNumVertices;
	unsigned int faceCount = objectMesh->mNumFaces;

	// Load triangles into indices vector
	indices.reserve(3 * faceCount);
	for (unsigned int i = 0; i < faceCount; i++) {
		// Model has triangulated faces after exporting from Blender
		indices.push_back(objectMesh->mFaces[i].mIndices[0]);
		indices.push_back(objectMesh->mFaces[i].mIndices[1]);
		indices.push_back(objectMesh->mFaces[i].mIndices[2]);
	}

	// Load vertex positions into vertex vector
	vertices.reserve(verticeCount);
	for (unsigned int i = 0; i < verticeCount; i++) {
		aiVector3D vertexPosition = objectMesh->mVertices[i];
		vertices.push_back(glm::vec3(vertexPosition.x, vertexPosition.y, vertexPosition.z));
	}

	// Load texture co-ordinates into uv vector
	uvs.reserve(verticeCount);
	for (unsigned int i = 0; i < verticeCount; i++) {
		// Each point has one set of UV co-ordinates
		aiVector3D UV = objectMesh->mTextureCoords[0][i];
		uvs.push_back(glm::vec2(UV.x, UV.y));
	}

	// Load vertex normals into normals vector
	normals.reserve(verticeCount);
	for (unsigned int i = 0; i < verticeCount; i++) {
		aiVector3D vertexNormal = objectMesh->mNormals[i];
		normals.push_back(glm::vec3(vertexNormal.x, vertexNormal.y, vertexNormal.z));
	}

	return true;
}