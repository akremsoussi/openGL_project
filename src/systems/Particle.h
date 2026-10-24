//-----------------------------------------------------------------------------
// One particle (CPU data only).  size = diameter of its square in world units.
//-----------------------------------------------------------------------------
#ifndef PARTICLE_H
#define PARTICLE_H

#include "glm/glm.hpp"

struct Particle
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);
	float size = 0.1f;
	float age = 0.0f;			// seconds since birth
	float lifetime = 1.0f;		// removed when age reaches it
};
#endif //PARTICLE_H
