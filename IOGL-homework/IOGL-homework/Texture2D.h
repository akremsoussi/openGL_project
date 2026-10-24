//-----------------------------------------------------------------------------
// Simple 2D texture class
//-----------------------------------------------------------------------------
#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#define GLEW_STATIC
#include "GL/glew.h"
#include <string>
using std::string;

class Texture2D
{
public:
	Texture2D();
	virtual ~Texture2D();

	bool loadTexture(const string& fileName, bool generateMipMaps = true);
	bool loadTextureFromMemory(const unsigned char* fileData, int size, bool generateMipMaps = true);	// ex: PNG inside a .glb
	void bind(GLuint texUnit = 0);
	void unbind(GLuint texUnit = 0);

private:
	Texture2D(const Texture2D& rhs) {}
	Texture2D& operator = (const Texture2D& rhs) { return *this; }

	void createTexture(unsigned char* imageData, int width, int height, bool generateMipMaps);

	GLuint mTexture;
};
#endif //TEXTURE2D_H
