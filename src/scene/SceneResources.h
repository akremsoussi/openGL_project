//-----------------------------------------------------------------------------
// Shaders and models used by the scene objects (the objects only point to
// them).  Create and destroy while the OpenGL context exists.
//-----------------------------------------------------------------------------
#ifndef SCENERESOURCES_H
#define SCENERESOURCES_H

#include "ShaderProgram.h"
#include "Mesh.h"
#include "graphics/SkinnedModel.h"

struct SceneResources
{
	// Shaders
	ShaderProgram lightingShader;		// OBJ models: lights + shadows
	ShaderProgram skinnedShader;		// animated characters: same lighting
	ShaderProgram bulbShader;			// lamps: plain (glowing) color
	ShaderProgram depthShader;			// OBJ models into a shadow map
	ShaderProgram skinnedDepthShader;	// animated characters into a shadow map
	ShaderProgram pointDepthShader;			// OBJ models into a lamp cube shadow map
	ShaderProgram skinnedPointDepthShader;	// animated characters into a lamp cube shadow map

	// Models
	Mesh map;							// OBJ + MTL + textures (homework loader)
	Mesh bulb;							// light bulb (homework model)
	SkinnedModel zed;					// glTF: recall animation (loops)
	SkinnedModel dancers[4];			// glTF: K/DA dancers beside Zed (dance loops)
	Mesh dead[5];						// OBJ: Qiyana and 4 champions lying dead (no animation)
	SkinnedModel trump;					// glTF: dancing at the red end of the lane (end of the tour)
};
#endif //SCENERESOURCES_H
