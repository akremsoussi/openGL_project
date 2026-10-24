//-----------------------------------------------------------------------------
// Timeline track that moves a CinematicCamera (its keyframes are set on the
// camera itself).  The camera must live longer than the track.
//-----------------------------------------------------------------------------
#ifndef CAMERATRACK_H
#define CAMERATRACK_H

#include "animation/Timeline.h"
#include "camera/CinematicCamera.h"

class CameraTrack : public TimelineTrack
{
public:
	explicit CameraTrack(CinematicCamera& camera) : mCamera(camera) {}

	virtual void apply(float time) { mCamera.update(time); }

private:
	CinematicCamera& mCamera;
};
#endif //CAMERATRACK_H
