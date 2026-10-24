//-----------------------------------------------------------------------------
// Scripted camera flight over the ARAM lane: keyframes of the cinematic
// camera, played by a timeline (key C).
//-----------------------------------------------------------------------------
#ifndef CAMERATOUR_H
#define CAMERATOUR_H

#include "animation/Timeline.h"
#include "camera/CinematicCamera.h"

namespace CameraTour
{
	const float DURATION = 46.0f;	// seconds

	// Sets the keyframes of the camera and adds its track to the timeline
	void build(Timeline& timeline, CinematicCamera& camera);
}
#endif //CAMERATOUR_H
