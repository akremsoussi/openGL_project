//-----------------------------------------------------------------------------
// Playback clock shared by several tracks
//-----------------------------------------------------------------------------
#include "animation/Timeline.h"

void Timeline::addTrack(std::unique_ptr<TimelineTrack> track)
{
	if (!track)
		return;

	track->apply(mTime);
	mTracks.push_back(std::move(track));
}

void Timeline::restart()
{
	mTime = 0.0f;
	mPlaying = true;
	for (auto& track : mTracks)
		track->apply(mTime);
}

void Timeline::update(float deltaTime)
{
	if (!mPlaying || deltaTime <= 0.0f)
		return;

	mTime += deltaTime;
	if (mDuration > 0.0f && mTime >= mDuration)
	{
		mTime = mDuration;
		mPlaying = false;
	}

	for (auto& track : mTracks)
		track->apply(mTime);
}
