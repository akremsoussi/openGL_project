//-----------------------------------------------------------------------------
// Draws all the particles in one call (instancing): one small square facing
// the camera per particle, with alpha blending.
//-----------------------------------------------------------------------------
#ifndef PARTICLERENDERER_H
#define PARTICLERENDERER_H

#include <vector>
#define GLEW_STATIC
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "ShaderProgram.h"
#include "systems/Particle.h"

class ParticleRenderer
{
public:
	 ParticleRenderer();
	~ParticleRenderer();

	bool init();
	void render(const std::vector<Particle>& particles, const glm::mat4& view, const glm::mat4& projection);

private:
	ParticleRenderer(const ParticleRenderer& rhs);
	ParticleRenderer& operator = (const ParticleRenderer& rhs);

	struct Instance			// per particle data sent to the GPU
	{
		float x, y, z, size;
		float opacity;
	};

	ShaderProgram mShader;
	GLuint mVAO, mBuffer;
	std::vector<Instance> mInstances;
};
#endif //PARTICLERENDERER_H
