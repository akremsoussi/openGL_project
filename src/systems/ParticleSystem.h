//-----------------------------------------------------------------------------
// Moves the particles and removes the old ones; drawing is done by
// ParticleRenderer (all particles in one instanced draw call).
//-----------------------------------------------------------------------------
#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <memory>
#include <vector>
#include "systems/Particle.h"

class ParticleRenderer;

class ParticleSystem
{
public:
	 ParticleSystem();
	~ParticleSystem();

	bool spawn(const Particle& particle);
	void update(float deltaTime);
	void clear() { mParticles.clear(); }
	size_t size() const { return mParticles.size(); }

	// OpenGL part: needs the context
	bool initGraphics();
	void releaseGraphics();
	void render(const glm::mat4& view, const glm::mat4& projection);

private:
	ParticleSystem(const ParticleSystem& rhs);
	ParticleSystem& operator = (const ParticleSystem& rhs);

	std::vector<Particle> mParticles;
	std::unique_ptr<ParticleRenderer> mRenderer;
};
#endif //PARTICLESYSTEM_H
