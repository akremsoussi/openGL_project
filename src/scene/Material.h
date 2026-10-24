//-----------------------------------------------------------------------------
// How an object is drawn: its shader, whether the scene lights apply, and
// extra uniforms (ex: the color of a lamp).  The textures and colors of the
// models themselves come from their files (.mtl, .glb).
//-----------------------------------------------------------------------------
#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <unordered_map>
#include "glm/glm.hpp"

class ShaderProgram;

struct Material
{
	ShaderProgram* shader = nullptr;
	bool receivesLighting = false;		// sun, point lights, flashlight and shadows

	std::unordered_map<std::string, float> floatUniforms;
	std::unordered_map<std::string, glm::vec3> vec3Uniforms;
	std::unordered_map<std::string, int> intUniforms;

	void setFloat(const std::string& name, float value) { floatUniforms[name] = value; }
	void setVec3(const std::string& name, const glm::vec3& value) { vec3Uniforms[name] = value; }
	void setInt(const std::string& name, int value) { intUniforms[name] = value; }
};
#endif //MATERIAL_H
