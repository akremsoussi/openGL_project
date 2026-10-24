//-----------------------------------------------------------------------------
// Playback clock shared by several tracks (camera, events...): each update
// advances the time and lets every track apply its value at that time.
//-----------------------------------------------------------------------------
#ifndef TIMELINE_H
#define TIMELINE_H

#include <memory>
#include <vector>

// Something animated by the timeline
class TimelineTrack
{
public:
	virtual ~TimelineTrack() {}
	virtual void apply(float time) = 0;
};

class Timeline
{
public:
	void play() { mPlaying = true; }
	void pause() { mPlaying = false; }
	void restart();					// back to 0 and play

	void update(float deltaTime);	// stops at the end (duration)

	float getTime() const { return mTime; }
	bool isPlaying() const { return mPlaying; }
	void setDuration(float duration) { mDuration = duration; }
	float getDuration() const { return mDuration; }

	void addTrack(std::unique_ptr<TimelineTrack> track);

private:
	float mTime = 0.0f;
	float mDuration = 0.0f;
	bool mPlaying = false;
	std::vector<std::unique_ptr<TimelineTrack> > mTracks;
};
#endif //TIMELINE_H
