//-----------------------------------------------------------------------------
// Basic Mesh class
//-----------------------------------------------------------------------------
#include "Mesh.h"
#include "ShaderProgram.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>


//-----------------------------------------------------------------------------
// split
//
// Params:  s - string to split
//		    t - string to split (ie. delimiter)
//
//Result:  Splits string according to some substring and returns it as a vector.
//-----------------------------------------------------------------------------
std::vector<std::string> split(std::string s, std::string t)
{
	std::vector<std::string> res;
	while(1)
	{
		size_t pos = s.find(t);
		if(pos == std::string::npos)
		{
			res.push_back(s);
			break;
		}
		res.push_back(s.substr(0, pos));
		s = s.substr(pos+1, s.size() - pos - 1);
	}
	return res;
}

//-----------------------------------------------------------------------------
// Returns the rest of the line (ex: a file name that may contain spaces)
// without the leading and trailing white spaces.
//-----------------------------------------------------------------------------
static std::string readRestOfLine(std::stringstream& ss)
{
	std::string rest;
	std::getline(ss >> std::ws, rest);

	size_t end = rest.find_last_not_of(" \t\r\n");
	return (end == std::string::npos) ? "" : rest.substr(0, end + 1);
}

//-----------------------------------------------------------------------------
// Converts an OBJ index (1-based, or negative = relative to the end of the
// list) to a 1-based index.  Returns 0 if there is no index.
//-----------------------------------------------------------------------------
static int objIndex(const std::string& s, size_t listSize)
{
	int index = 0;
	if (s.size() > 0)
		sscanf(s.c_str(), "%d", &index);

	if (index < 0)
		index = (int)listSize + index + 1;

	return index;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Mesh::Mesh()
	:mLoaded(false), mVBO(0), mVAO(0)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{
	glDeleteVertexArrays(1, &mVAO);
	glDeleteBuffers(1, &mVBO);

	std::map<std::string, Texture2D*>::iterator it;
	for (it = mTextures.begin(); it != mTextures.end(); ++it)
		delete it->second;
}

//-----------------------------------------------------------------------------
// Loads a Wavefront OBJ model
//
// NOTE: This is not a complete, full featured OBJ loader.  It is greatly
// simplified.
// Assumptions!
//  - Faces with more than 3 vertices are split into triangles (fan)
//  - only commands "v", "vt", "vn", "f", "mtllib" and "usemtl" are supported
//  - Materials (.mtl) are optional.  Use draw(shader) to render with them.
//-----------------------------------------------------------------------------
bool Mesh::loadOBJ(const std::string& filename)
{
	std::vector<int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> tempVertices;
	std::vector<glm::vec2> tempUVs;
	std::vector<glm::vec3> tempNormals;


	if (filename.find(".obj") != std::string::npos)
	{
		std::ifstream fin(filename, std::ios::in);
		if (!fin)
		{
			std::cerr << "Cannot open " << filename << std::endl;
			return false;
		}

		std::cout << "Loading OBJ file " << filename << " ..." << std::endl;

		// The .mtl and texture files are relative to the OBJ file folder
		size_t slash = filename.find_last_of("/\\");
		mDirectory = (slash == std::string::npos) ? "" : filename.substr(0, slash + 1);

		std::string lineBuffer;
		while (std::getline(fin, lineBuffer))
		{
			std::stringstream ss(lineBuffer);
			std::string cmd;
			ss >> cmd;

			if (cmd == "v")
			{
				glm::vec3 vertex;
				int dim = 0;
				while (dim < 3 && ss >> vertex[dim])
					dim++;

				tempVertices.push_back(vertex);
			}
			else if (cmd == "vt")
			{
				glm::vec2 uv;
				int dim = 0;
				while (dim < 2 && ss >> uv[dim])
					dim++;

				tempUVs.push_back(uv);
			}
			else if (cmd == "vn")
			{
				glm::vec3 normal;
				int dim = 0;
				while (dim < 3 && ss >> normal[dim])
					dim++;
				normal = glm::normalize(normal);
				tempNormals.push_back(normal);
			}
			else if (cmd == "f")
			{
				std::string faceData;
				std::vector<int> faceVertices, faceUVs, faceNormals;

				while (ss>>faceData)
				{
					std::vector<std::string> data = split(faceData, "/");

					faceVertices.push_back(objIndex(data[0], tempVertices.size()));

					// Is face format v//vn?  If data[1] is empty string then
					// this vertex has no texture coordinate
					faceUVs.push_back(data.size() >= 2 ? objIndex(data[1], tempUVs.size()) : 0);

					// Does this vertex have a normal?
					faceNormals.push_back(data.size() >= 3 ? objIndex(data[2], tempNormals.size()) : 0);
				}

				// Split the polygon into triangles (0, i, i+1)
				for (size_t i = 1; i + 1 < faceVertices.size(); i++)
				{
					size_t corners[3] = { 0, i, i + 1 };
					for (int c = 0; c < 3; c++)
					{
						vertexIndices.push_back(faceVertices[corners[c]]);
						uvIndices.push_back(faceUVs[corners[c]]);
						normalIndices.push_back(faceNormals[corners[c]]);
					}
				}
			}
			else if (cmd == "mtllib")
			{
				loadMTL(mDirectory + readRestOfLine(ss));
			}
			else if (cmd == "usemtl")
			{
				// Faces read before the first "usemtl" have no material
				if (mSubMeshes.empty())
				{
					SubMesh noMaterial = { 0, 0, -1 };
					mSubMeshes.push_back(noMaterial);
				}

				// Close the current part and start a new one with this material
				mSubMeshes.back().count = vertexIndices.size() - mSubMeshes.back().start;

				SubMesh part = { (unsigned int)vertexIndices.size(), 0, findMaterial(readRestOfLine(ss)) };
				mSubMeshes.push_back(part);
			}
		}

		// Close the file
		fin.close();

		// Close the last part and remove the empty ones
		if (!mSubMeshes.empty())
			mSubMeshes.back().count = vertexIndices.size() - mSubMeshes.back().start;

		for (size_t i = mSubMeshes.size(); i-- > 0; )
		{
			if (mSubMeshes[i].count == 0)
				mSubMeshes.erase(mSubMeshes.begin() + i);
		}


		// For each vertex of each triangle
		for (unsigned int i = 0; i < vertexIndices.size(); i++)
		{
			Vertex meshVertex;
			meshVertex.position = glm::vec3(0.0f);
			meshVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			meshVertex.texCoords = glm::vec2(0.0f);

			// Get the attributes using the indices

			if (vertexIndices[i] > 0 && vertexIndices[i] <= (int)tempVertices.size())
			{
				glm::vec3 vertex = tempVertices[vertexIndices[i] - 1];
				meshVertex.position = vertex;
			}

			if (normalIndices[i] > 0 && normalIndices[i] <= (int)tempNormals.size())
			{
				glm::vec3 normal = tempNormals[normalIndices[i] - 1];
				meshVertex.normal = normal;
			}

			if (uvIndices[i] > 0 && uvIndices[i] <= (int)tempUVs.size())
			{
				glm::vec2 uv = tempUVs[uvIndices[i] - 1];
				meshVertex.texCoords = uv;
			}

			mVertices.push_back(meshVertex);
		}

		if (mVertices.empty())
		{
			std::cerr << "No faces found in " << filename << std::endl;
			return false;
		}

		std::cout << "  " << mVertices.size() / 3 << " triangles, "
			<< mMaterials.size() << " materials, "
			<< mSubMeshes.size() << " parts" << std::endl;

		// Create and initialize the buffers
		initBuffers();

		return (mLoaded = true);
	}

	// We shouldn't get here so return failure
	return false;
}

//-----------------------------------------------------------------------------
// Loads the materials of a Wavefront MTL file
//
// Supported commands: "newmtl", "Ka", "Kd", "Ks", "Ns" and "map_Kd"
//-----------------------------------------------------------------------------
bool Mesh::loadMTL(const std::string& filename)
{
	std::ifstream fin(filename, std::ios::in);
	if (!fin)
	{
		std::cerr << "Cannot open " << filename << std::endl;
		return false;
	}

	std::cout << "Loading MTL file " << filename << " ..." << std::endl;

	std::string lineBuffer;
	while (std::getline(fin, lineBuffer))
	{
		std::stringstream ss(lineBuffer);
		std::string cmd;
		ss >> cmd;

		if (cmd == "newmtl")
		{
			MeshMaterial material;
			material.name = readRestOfLine(ss);
			material.ambient = glm::vec3(1.0f);
			material.diffuse = glm::vec3(0.8f);
			material.specular = glm::vec3(0.0f);
			material.shininess = 32.0f;
			material.diffuseMap = NULL;
			mMaterials.push_back(material);
		}
		else if (mMaterials.empty())
		{
			// Ignore everything before the first material
			continue;
		}
		else if (cmd == "Ka")
		{
			glm::vec3& ka = mMaterials.back().ambient;
			ss >> ka.r >> ka.g >> ka.b;
		}
		else if (cmd == "Kd")
		{
			glm::vec3& kd = mMaterials.back().diffuse;
			ss >> kd.r >> kd.g >> kd.b;
		}
		else if (cmd == "Ks")
		{
			glm::vec3& ks = mMaterials.back().specular;
			ss >> ks.r >> ks.g >> ks.b;
		}
		else if (cmd == "Ns")
		{
			float ns = 0.0f;
			ss >> ns;
			// pow(x, 0) = 1 would make the whole surface shine
			mMaterials.back().shininess = glm::max(ns, 1.0f);
		}
		else if (cmd == "map_Kd")
		{
			// Options (ex: "-bm 1.0") may come before the file name, which is the last word
			std::string word, path;
			while (ss >> word)
				path = word;

			mMaterials.back().diffuseMap = loadMaterialTexture(path);
		}
	}

	fin.close();
	return true;
}

//-----------------------------------------------------------------------------
// Loads (once) a texture used by a material.
//
// Exporters often write absolute paths from the artist's machine
// (ex: "C:/wood.png") so we also look for the file name alone in the model
// folder and in its "textures" sub folder.
//-----------------------------------------------------------------------------
Texture2D* Mesh::loadMaterialTexture(const std::string& path)
{
	std::map<std::string, Texture2D*>::iterator it = mTextures.find(path);
	if (it != mTextures.end())
		return it->second;

	std::string fileName = path.substr(path.find_last_of("/\\") + 1);
	std::string candidates[] = {
		mDirectory + path,
		path,
		mDirectory + fileName,
		mDirectory + "textures/" + fileName
	};

	Texture2D* texture = NULL;
	for (int i = 0; i < 4 && texture == NULL; i++)
	{
		if (!std::ifstream(candidates[i].c_str()))
			continue;

		texture = new Texture2D();
		if (!texture->loadTexture(candidates[i], true))
		{
			delete texture;
			texture = NULL;
		}
	}

	if (texture == NULL)
		std::cerr << "Cannot find texture '" << path << "'" << std::endl;

	mTextures[path] = texture;
	return texture;
}

//-----------------------------------------------------------------------------
// Returns the index of the material with this name, or -1
//-----------------------------------------------------------------------------
int Mesh::findMaterial(const std::string& name) const
{
	for (size_t i = 0; i < mMaterials.size(); i++)
	{
		if (mMaterials[i].name == name)
			return (int)i;
	}

	std::cerr << "Unknown material '" << name << "'" << std::endl;
	return -1;
}

//-----------------------------------------------------------------------------
// Create and initialize the vertex buffer and vertex array object
// Must have valid, non-empty std::vector of Vertex objects.
//-----------------------------------------------------------------------------
void Mesh::initBuffers()
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);

	// Vertex Positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normals attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Vertex Texture Coords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// unbind to make sure other code does not change it somewhere else
	glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// Render the mesh
//-----------------------------------------------------------------------------
void Mesh::draw()
{
	if (!mLoaded) return;

	glBindVertexArray(mVAO);
	glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
	glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// Render the mesh with its materials.
// The shader must be active and have a "material" uniform struct with
// ambient, diffuse, specular, shininess, hasDiffuseMap and diffuseMap
// (sampler on texture unit 0).
//-----------------------------------------------------------------------------
void Mesh::draw(ShaderProgram& shader)
{
	if (!mLoaded) return;

	// No "usemtl" in the file: nothing to set, draw everything
	if (mSubMeshes.empty())
	{
		draw();
		return;
	}

	MeshMaterial defaultMaterial;
	defaultMaterial.ambient = glm::vec3(1.0f);
	defaultMaterial.diffuse = glm::vec3(0.8f);
	defaultMaterial.specular = glm::vec3(0.0f);
	defaultMaterial.shininess = 32.0f;
	defaultMaterial.diffuseMap = NULL;

	glBindVertexArray(mVAO);
	for (size_t i = 0; i < mSubMeshes.size(); i++)
	{
		const SubMesh& part = mSubMeshes[i];
		const MeshMaterial& material = (part.materialIndex >= 0) ? mMaterials[part.materialIndex] : defaultMaterial;

		shader.setUniform("material.ambient", material.ambient);
		shader.setUniform("material.diffuse", material.diffuse);
		shader.setUniform("material.specular", material.specular);
		shader.setUniform("material.shininess", material.shininess);
		shader.setUniform("material.hasDiffuseMap", (GLint)(material.diffuseMap != NULL));

		if (material.diffuseMap != NULL)
			material.diffuseMap->bind(0);

		glDrawArrays(GL_TRIANGLES, part.start, part.count);
	}
	glBindVertexArray(0);
}
