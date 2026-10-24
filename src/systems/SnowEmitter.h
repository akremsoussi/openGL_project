//-----------------------------------------------------------------------------
// Continuous snowfall: creates snowflakes in a box above the camera so that
// it always snows around the viewer.  Random numbers come from a fixed seed,
// so the snow is the same at every run.
//-----------------------------------------------------------------------------
#ifndef SNOWEMITTER_H
#define SNOWEMITTER_H

#include <cstdint>
#include <random>
#include "glm/glm.hpp"
#include "systems/ParticleSystem.h"

struct SnowConfig
{
	float flakesPerSecond = 1200.0f;
	float halfWidth = 40.0f;					// box around the camera (x and z)
	float minHeight = 4.0f, maxHeight = 25.0f;	// birth height above the camera
	float fallBelow = 6.0f;						// dies this far below the camera
	float minSpeed = 1.5f, maxSpeed = 3.0f;		// falling speed
	glm::vec3 wind = glm::vec3(0.6f, 0.0f, 0.3f);
	float minSize = 0.10f, maxSize = 0.22f;
	std::uint32_t seed = 7;
};

class SnowEmitter
{
public:
	explicit SnowEmitter(ParticleSystem& system);

	void configure(const SnowConfig& config);

	// Fills the sky at once (flakes already at every stage of their fall)
	void prefill(const glm::vec3& center);

	// Creates the flakes of this frame around "center" (the camera)
	void update(float deltaTime, const glm::vec3& center);

private:
	Particle makeFlake(const glm::vec3& center, float age01);
	float random(float minimum, float maximum);

	ParticleSystem& mSystem;
	SnowConfig mConfig;
	std::mt19937 mRandom;
	float mPending;			// fraction of a flake left from the previous frames
};
#endif //SNOWEMITTER_H
