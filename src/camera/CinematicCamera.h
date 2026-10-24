//-----------------------------------------------------------------------------
// Camera moved by keyframes: its position, the point it looks at and its
// field of view each follow an AnimationTrack.  Derived from the homework
// Camera class (same view matrix).
//-----------------------------------------------------------------------------
#ifndef CINEMATICCAMERA_H
#define CINEMATICCAMERA_H

#include "Camera.h"
#include "animation/AnimationTrack.h"

class CinematicCamera : public Camera
{
public:
	CinematicCamera();

	void update(float time);	// pose at this time of the animation

	AnimationTrack<glm::vec3>& positionTrack() { return mPositionTrack; }
	AnimationTrack<glm::vec3>& targetTrack() { return mTargetTrack; }
	AnimationTrack<float>& fovTrack() { return mFovTrack; }

private:
	virtual void updateCameraVectors();

	AnimationTrack<glm::vec3> mPositionTrack;
	AnimationTrack<glm::vec3> mTargetTrack;
	AnimationTrack<float> mFovTrack;
};
#endif //CINEMATICCAMERA_H
