//-----------------------------------------------------------------------------
// Easing curves (quadratic)
//-----------------------------------------------------------------------------
#include "animation/Easing.h"
#include <algorithm>

float Easing::apply(float t, EasingType type)
{
	t = std::min(std::max(t, 0.0f), 1.0f);

	switch (type)
	{
	case EasingType::EaseIn:
		return t * t;
	case EasingType::EaseOut:
		return 1.0f - (1.0f - t) * (1.0f - t);
	case EasingType::EaseInOut:
		return (t < 0.5f) ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
	case EasingType::Linear:
	default:
		return t;
	}
}
