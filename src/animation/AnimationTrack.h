//-----------------------------------------------------------------------------
// Keyframes of one value over time (float, vec3...) and its value at any time:
// interpolation between the two keyframes around it, with the easing of the
// first one.  Keyframes must be added in time order.
//-----------------------------------------------------------------------------
#ifndef ANIMATIONTRACK_H
#define ANIMATIONTRACK_H

#include <vector>
#include "animation/Easing.h"

template <typename T>
struct Keyframe
{
	float time;
	T value;
	EasingType easing;		// from this keyframe to the next
};

template <typename T>
class AnimationTrack
{
public:
	void addKeyframe(float time, const T& value, EasingType easing = EasingType::Linear)
	{
		Keyframe<T> keyframe = { time, value, easing };
		mKeyframes.push_back(keyframe);
	}

	T evaluate(float time) const
	{
		if (mKeyframes.empty())
			return T();
		if (time <= mKeyframes.front().time)
			return mKeyframes.front().value;
		if (time >= mKeyframes.back().time)
			return mKeyframes.back().value;

		for (size_t i = 0; i + 1 < mKeyframes.size(); i++)
		{
			const Keyframe<T>& a = mKeyframes[i];
			const Keyframe<T>& b = mKeyframes[i + 1];
			if (time >= a.time && time <= b.time)
			{
				float progress = Easing::apply((time - a.time) / (b.time - a.time), a.easing);
				return a.value + (b.value - a.value) * progress;
			}
		}
		return mKeyframes.back().value;
	}

	bool empty() const { return mKeyframes.empty(); }

private:
	std::vector<Keyframe<T> > mKeyframes;
};
#endif //ANIMATIONTRACK_H
