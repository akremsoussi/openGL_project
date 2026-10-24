//-----------------------------------------------------------------------------
// Camera moved by keyframes
//-----------------------------------------------------------------------------
#include "camera/CinematicCamera.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CinematicCamera::CinematicCamera()
{
	mPosition = glm::vec3(0.0f, 0.0f, 10.0f);
	mTargetPos = glm::vec3(0.0f);
	updateCameraVectors();
}

//-----------------------------------------------------------------------------
// Reads the tracks at this time
//-----------------------------------------------------------------------------
void CinematicCamera::update(float time)
{
	if (!mPositionTrack.empty())
		mPosition = mPositionTrack.evaluate(time);
	if (!mTargetTrack.empty())
		mTargetPos = mTargetTrack.evaluate(time);
	if (!mFovTrack.empty())
		mFOV = mFovTrack.evaluate(time);

	updateCameraVectors();
}

//-----------------------------------------------------------------------------
// Look, right and up vectors from the position and the target
//-----------------------------------------------------------------------------
void CinematicCamera::updateCameraVectors()
{
	glm::vec3 direction = mTargetPos - mPosition;
	if (glm::length(direction) < 0.0001f)
		return;

	mLook = glm::normalize(direction);
	mRight = glm::normalize(glm::cross(mLook, WORLD_UP));
	mUp = glm::normalize(glm::cross(mRight, mLook));
}
