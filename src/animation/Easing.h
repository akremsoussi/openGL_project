//-----------------------------------------------------------------------------
// Easing curves: how a value goes from one keyframe to the next
// (t in [0, 1] -> progress in [0, 1])
//-----------------------------------------------------------------------------
#ifndef EASING_H
#define EASING_H

enum class EasingType
{
	Linear,		// constant speed
	EaseIn,		// starts slowly
	EaseOut,	// ends slowly
	EaseInOut	// starts and ends slowly
};

namespace Easing
{
	float apply(float t, EasingType type);
}
#endif //EASING_H
