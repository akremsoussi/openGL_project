//-----------------------------------------------------------------------------
// Draws the scene objects: each with its material (shader, lights, uniforms)
// and into the shadow maps.
//-----------------------------------------------------------------------------
#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "glm/glm.hpp"
#include "scene/Scene.h"
#include "scene/LightManager.h"

class ShadowMap;
class PointShadowMap;

class Renderer
{
public:
	// Shadow maps used by the lit materials.  Texture units: 1 moon (map),
	// 2 moon (characters), 3 flashlight, 4-6 lamps (static), 7-9 lamps (moving)
	void setShadowMaps(ShadowMap* mapShadow, ShadowMap* characterShadow, bool enabled);
	void setSpotShadow(ShadowMap* spotShadow, bool on);
	void setPointShadows(const std::vector<PointShadowMap*>& staticMaps,
	                     const std::vector<PointShadowMap*>& dynamicMaps, float farPlane);

	// Draws the visible objects seen from the camera
	void render(Scene& scene, const LightManager& lights, const glm::mat4& view,
	            const glm::mat4& projection, const glm::vec3& cameraPosition);

	// Moon shadows: depth of the objects of one shadow group into a shadow map.
	// viewportWidth/Height: window framebuffer size to restore afterwards.
	void renderShadows(Scene& scene, ShadowGroup group, ShadowMap& shadowMap,
	                   int viewportWidth, int viewportHeight);

	// Flashlight shadows: depth of every shadow casting object
	void renderSpotShadows(Scene& scene, ShadowMap& shadowMap, int viewportWidth, int viewportHeight);

	// Lamp shadows: distance of the objects to the lamp, in the 6 faces of its
	// cube map.  animated = false: still objects (drawn once);
	// animated = true: moving characters closer than "range" (every frame).
	void renderPointShadows(Scene& scene, PointShadowMap& shadowMap, bool animated, float range,
	                        int viewportWidth, int viewportHeight);

private:
	void renderObject(SceneObject& object, const LightManager& lights, const glm::mat4& view,
	                  const glm::mat4& projection, const glm::vec3& cameraPosition);
	void applyShadows(ShaderProgram& shader);

	ShadowMap* mMapShadow = nullptr;
	ShadowMap* mCharacterShadow = nullptr;
	bool mShadowsOn = false;

	ShadowMap* mSpotShadow = nullptr;
	bool mSpotShadowOn = false;

	std::vector<PointShadowMap*> mPointStatic;
	std::vector<PointShadowMap*> mPointDynamic;
	float mPointFar = 100.0f;
};
#endif //RENDERER_H
