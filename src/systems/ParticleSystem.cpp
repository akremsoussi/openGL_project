//-----------------------------------------------------------------------------
// Moves the particles and removes the old ones
//-----------------------------------------------------------------------------
#include "systems/ParticleSystem.h"
#include "graphics/ParticleRenderer.h"
#include <algorithm>

ParticleSystem::ParticleSystem() = default;
ParticleSystem::~ParticleSystem() = default;

//-----------------------------------------------------------------------------
// Adds a particle (refused if its values make no sense)
//-----------------------------------------------------------------------------
bool ParticleSystem::spawn(const Particle& particle)
{
	if (particle.size <= 0.0f || particle.age < 0.0f || particle.lifetime <= particle.age)
		return false;

	mParticles.push_back(particle);
	return true;
}

//-----------------------------------------------------------------------------
// Moves every particle along its velocity, then removes the dead ones
//-----------------------------------------------------------------------------
void ParticleSystem::update(float deltaTime)
{
	if (deltaTime <= 0.0f)
		return;

	for (Particle& particle : mParticles)
	{
		particle.position += particle.velocity * std::min(deltaTime, particle.lifetime - particle.age);
		particle.age = std::min(particle.lifetime, particle.age + deltaTime);
	}

	mParticles.erase(std::remove_if(mParticles.begin(), mParticles.end(),
		[](const Particle& particle) { return particle.age >= particle.lifetime; }), mParticles.end());
}

//-----------------------------------------------------------------------------
// OpenGL part
//-----------------------------------------------------------------------------
bool ParticleSystem::initGraphics()
{
	if (mRenderer)
		return true;

	std::unique_ptr<ParticleRenderer> renderer(new ParticleRenderer());
	if (!renderer->init())
		return false;

	mRenderer = std::move(renderer);
	return true;
}

void ParticleSystem::releaseGraphics()
{
	mRenderer.reset();
}

void ParticleSystem::render(const glm::mat4& view, const glm::mat4& projection)
{
	if (mRenderer)
		mRenderer->render(mParticles, view, projection);
}
