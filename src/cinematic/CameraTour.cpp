//-----------------------------------------------------------------------------
// Scripted camera flight over the ARAM lane
//
// The lane goes from the blue base (+Z) to the red base (-Z) around x = -5,
// floor at y = 6.  Zed and the others are in the middle (z = 0 to 10), Trump
// at the red end: the tour ends on him dancing and stays there.
//-----------------------------------------------------------------------------
#include "cinematic/CameraTour.h"
#include "animation/CameraTrack.h"
#include "scene/SceneSetup.h"

namespace
{
	const glm::vec3 TRUMP = SceneSetup::TRUMP_POSITION;

	struct Shot
	{
		float time;
		glm::vec3 position;
		glm::vec3 target;		// point looked at
		float fov;				// degrees
		EasingType easing;		// toward the next shot
	};

	const Shot SHOTS[] = {
		// High above the blue base, looking down the lane
		{  0.0f, glm::vec3( -5.0f, 60.0f, 150.0f), glm::vec3(-5.0f,  6.0f,  60.0f), 50.0f, EasingType::EaseInOut },
		// Down to the first towers
		{  7.0f, glm::vec3( -5.0f, 22.0f, 120.0f), glm::vec3(-5.0f,  8.0f,  50.0f), 50.0f, EasingType::Linear },
		// Along the lane, beside the towers
		{ 13.0f, glm::vec3( 12.0f, 14.0f,  55.0f), glm::vec3(-5.0f,  9.0f,  15.0f), 45.0f, EasingType::EaseOut },
		// Close to Zed and Qiyana, from the front
		{ 18.0f, glm::vec3(  3.0f,  9.5f,  13.0f), glm::vec3(-5.0f,  8.0f,   2.0f), 40.0f, EasingType::EaseInOut },
		// Around them: side
		{ 23.0f, glm::vec3(-17.0f, 10.0f,   5.0f), glm::vec3(-5.0f,  7.5f,   2.0f), 40.0f, EasingType::EaseInOut },
		// Behind Zed
		{ 28.0f, glm::vec3( -6.0f, 13.0f, -12.0f), glm::vec3(-5.0f,  7.5f,   2.0f), 45.0f, EasingType::EaseIn },
		// Over the red side, Trump in sight (passing beside the red lamp)
		{ 34.0f, glm::vec3(  3.0f, 30.0f, -70.0f), TRUMP + glm::vec3(0.0f, 2.0f, 0.0f), 50.0f, EasingType::EaseInOut },
		// Close on Trump dancing
		{ 40.0f, TRUMP + glm::vec3( 2.0f, 3.2f, 11.0f), TRUMP + glm::vec3(0.0f, 1.8f, 0.0f), 40.0f, EasingType::Linear },
		// Slow drift around him: the tour ends (and stays) here
		{ 46.0f, TRUMP + glm::vec3(-3.0f, 2.6f,  9.0f), TRUMP + glm::vec3(0.0f, 1.6f, 0.0f), 36.0f, EasingType::Linear }
	};
}

//-----------------------------------------------------------------------------
// One keyframe per shot on each camera track
//-----------------------------------------------------------------------------
void CameraTour::build(Timeline& timeline, CinematicCamera& camera)
{
	for (const Shot& shot : SHOTS)
	{
		camera.positionTrack().addKeyframe(shot.time, shot.position, shot.easing);
		camera.targetTrack().addKeyframe(shot.time, shot.target, shot.easing);
		camera.fovTrack().addKeyframe(shot.time, shot.fov, shot.easing);
	}

	timeline.setDuration(DURATION);
	timeline.addTrack(std::unique_ptr<TimelineTrack>(new CameraTrack(camera)));
}
