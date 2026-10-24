//-----------------------------------------------------------------------------
// Continuous snowfall around the camera
//-----------------------------------------------------------------------------
#include "systems/SnowEmitter.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
SnowEmitter::SnowEmitter(ParticleSystem& system)
	: mSystem(system), mPending(0.0f)
{
	mRandom.seed(mConfig.seed);
}

void SnowEmitter::configure(const SnowConfig& config)
{
	mConfig = config;
	mRandom.seed(mConfig.seed);
	mPending = 0.0f;
}

//-----------------------------------------------------------------------------
// Random number in [minimum, maximum]
//-----------------------------------------------------------------------------
float SnowEmitter::random(float minimum, float maximum)
{
	std::uniform_real_distribution<float> distribution(minimum, maximum);
	return distribution(mRandom);
}

//-----------------------------------------------------------------------------
// One flake: born above the camera, falls with the wind until it is a bit
// below the camera.  age01 = how far it already is in its fall (0 = new).
//-----------------------------------------------------------------------------
Particle SnowEmitter::makeFlake(const glm::vec3& center, float age01)
{
	glm::vec3 birth = center + glm::vec3(random(-mConfig.halfWidth, mConfig.halfWidth),
	                                     random(mConfig.minHeight, mConfig.maxHeight),
	                                     random(-mConfig.halfWidth, mConfig.halfWidth));

	Particle flake;
	flake.velocity = mConfig.wind + glm::vec3(0.0f, -random(mConfig.minSpeed, mConfig.maxSpeed), 0.0f);
	flake.size = random(mConfig.minSize, mConfig.maxSize);
	flake.lifetime = (birth.y - center.y + mConfig.fallBelow) / -flake.velocity.y;
	flake.age = age01 * flake.lifetime;
	flake.position = birth + flake.velocity * flake.age;
	return flake;
}

//-----------------------------------------------------------------------------
// As many flakes as would be alive after snowing for a while
//-----------------------------------------------------------------------------
void SnowEmitter::prefill(const glm::vec3& center)
{
	float averageHeight = (mConfig.minHeight + mConfig.maxHeight) * 0.5f + mConfig.fallBelow;
	float averageLifetime = averageHeight / ((mConfig.minSpeed + mConfig.maxSpeed) * 0.5f);
	int count = (int)(mConfig.flakesPerSecond * averageLifetime);

	for (int i = 0; i < count; i++)
		mSystem.spawn(makeFlake(center, random(0.0f, 0.99f)));
}

//-----------------------------------------------------------------------------
// flakesPerSecond x deltaTime new flakes (the fraction is kept for later)
//-----------------------------------------------------------------------------
void SnowEmitter::update(float deltaTime, const glm::vec3& center)
{
	if (deltaTime <= 0.0f)
		return;

	mPending += mConfig.flakesPerSecond * deltaTime;
	while (mPending >= 1.0f)
	{
		mSystem.spawn(makeFlake(center, 0.0f));
		mPending -= 1.0f;
	}
}
