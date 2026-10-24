//-----------------------------------------------------------------------------
// Builds the ARAM scene: loads the resources, places the objects and the
// lights.  All the values that describe this scene are here.
//-----------------------------------------------------------------------------
#ifndef SCENESETUP_H
#define SCENESETUP_H

#include "glm/glm.hpp"
#include "scene/Scene.h"
#include "scene/LightManager.h"
#include "scene/SceneResources.h"

namespace SceneSetup
{
	// Directional light (moon by night, sun by day) coming from the side and
	// rather low (~37 degrees) so that the shadows are long and visible.
	const glm::vec3 LIGHT_DIRECTION(-0.7f, -0.6f, -0.35f);

	// Shadow map of the map (static, rendered once): covers the lane
	const glm::vec3 MAP_SHADOW_MIN(-75.0f, -20.0f, -215.0f);
	const glm::vec3 MAP_SHADOW_MAX( 60.0f,  70.0f,  215.0f);
	const int MAP_SHADOW_SIZE = 4096;

	// Shadow map of the characters (animated, rendered every frame): around
	// Zed, Qiyana and the dancers (x from -10 to 0)
	const glm::vec3 CHARACTER_SHADOW_MIN(-19.0f, -1.0f, -6.0f);
	const glm::vec3 CHARACTER_SHADOW_MAX(  4.0f, 13.0f,  9.0f);
	const int CHARACTER_SHADOW_SIZE = 2048;

	// First person camera start: blue side of the lane, looking toward the red side
	const glm::vec3 CAMERA_START(-5.0f, 9.0f, 110.0f);

	// Trump at the red end of the lane (also used by the camera tour)
	const glm::vec3 TRUMP_POSITION(-5.0f, 6.9f, -112.0f);
	const float TRUMP_ROTATION = 0.0f;			// degrees around Y

	// Sound played in a loop (key M: mute)
	const char* const SOUNDTRACK = "resources/sounds/KDA_Recall_SFX.mp3";

	// Loads everything and fills the scene and the lights.  False if the map is missing.
	bool build(Scene& scene, LightManager& lights, SceneResources& resources);

	// Night: cold moonlight and brighter lamps.  Day: sun.
	void setTimeOfDay(LightManager& lights, bool night);
	glm::vec4 skyColor(bool night);
	glm::vec3 skyLightColor(bool night);	// moon / sun disk (shows where the main light comes from)

	// Lamp (point light) shadows: cube maps reaching POINT_SHADOW_FAR from the
	// lamp; moving characters closer than POINT_SHADOW_DYNAMIC_RANGE are
	// redrawn every frame (the rest is drawn once)
	const int POINT_SHADOW_SIZE = 1024;
	const int POINT_SHADOW_DYNAMIC_SIZE = 512;
	const float POINT_SHADOW_FAR = 150.0f;
	const float POINT_SHADOW_DYNAMIC_RANGE = 50.0f;

	// Flashlight shadow map (perspective, redrawn every frame while it is on)
	const int SPOT_SHADOW_SIZE = 2048;
	const float SPOT_SHADOW_FOV = 50.0f;		// a bit wider than the 40 degree cone
	const float SPOT_SHADOW_FAR = 150.0f;
}
#endif //SCENESETUP_H
