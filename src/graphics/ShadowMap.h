//-----------------------------------------------------------------------------
// Shadow map: depth texture rendered from the point of view of a directional
// light, then used by the scene shader to know which fragments are in shadow.
//-----------------------------------------------------------------------------
#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "glm/glm.hpp"

class ShadowMap
{
public:
	 ShadowMap();
	~ShadowMap();

	bool init(int size);

	// Builds the light view/projection matrix so that the box (world space)
	// [boxMin, boxMax] is fully inside the shadow map.
	void setLight(const glm::vec3& lightDirection, const glm::vec3& boxMin, const glm::vec3& boxMax);

	// Spot light (flashlight): perspective projection from the light position,
	// looking along its direction, wide enough for its cone
	void setSpotLight(const glm::vec3& position, const glm::vec3& direction, float fovDegrees, float nearPlane, float farPlane);
	const glm::mat4& getLightSpaceMatrix() const { return mLightSpaceMatrix; }

	// Render pass into the depth texture.  end() restores the window framebuffer.
	void begin();
	void end(int viewportWidth, int viewportHeight);

	void bind(GLuint texUnit);

private:
	ShadowMap(const ShadowMap& rhs);
	ShadowMap& operator = (const ShadowMap& rhs);

	int mSize;
	GLuint mFBO;
	GLuint mDepthTexture;
	glm::mat4 mLightSpaceMatrix;
};
#endif //SHADOWMAP_H
