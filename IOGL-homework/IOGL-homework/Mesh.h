//-----------------------------------------------------------------------------
// Basic Mesh class
//-----------------------------------------------------------------------------
#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <map>

#define GLEW_STATIC
#include "GL/glew.h"	// Important - this header must come before glfw3 header
#include "glm/glm.hpp"
#include "Texture2D.h"

class ShaderProgram;


struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

// Material read from a Wavefront .mtl file
struct MeshMaterial
{
	std::string name;
	glm::vec3 ambient;		// Ka
	glm::vec3 diffuse;		// Kd (used when there is no diffuse map)
	glm::vec3 specular;		// Ks
	float shininess;		// Ns
	Texture2D* diffuseMap;	// map_Kd (NULL if the material has no texture)
};

// Range of vertices drawn with the same material (one "usemtl" block of the OBJ)
struct SubMesh
{
	unsigned int start;
	unsigned int count;
	int materialIndex;		// -1 if no material
};

class Mesh
{
public:

	 Mesh();
	~Mesh();

	bool loadOBJ(const std::string& filename);
	void draw();
	void draw(ShaderProgram& shader);	// draw each part with its own .mtl material

private:

	// A mesh owns OpenGL objects and textures so it must not be copied
	Mesh(const Mesh& rhs);
	Mesh& operator = (const Mesh& rhs);

	void initBuffers();
	bool loadMTL(const std::string& filename);
	Texture2D* loadMaterialTexture(const std::string& path);
	int findMaterial(const std::string& name) const;

	bool mLoaded;
	std::vector<Vertex> mVertices;
	GLuint mVBO, mVAO;

	std::string mDirectory;		// folder of the OBJ file, used to find the .mtl and the textures
	std::vector<MeshMaterial> mMaterials;
	std::vector<SubMesh> mSubMeshes;
	std::map<std::string, Texture2D*> mTextures;	// textures already loaded, by .mtl path
};
#endif //MESH_H
