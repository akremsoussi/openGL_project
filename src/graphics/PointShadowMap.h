//-----------------------------------------------------------------------------
// Shadow map of a point light (a lamp shines in every direction): a cube map
// of 6 square images, one per direction (+X, -X, +Y, -Y, +Z, -Z), each seen
// from the light with a 90 degree perspective.  Each texel stores the distance
// from the light to the closest object, divided by farPlane.
//-----------------------------------------------------------------------------
#ifndef POINTSHADOWMAP_H
#define POINTSHADOWMAP_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "glm/glm.hpp"

class PointShadowMap
{
public:
	 PointShadowMap();
	~PointShadowMap();

	bool init(int size, float farPlane);
	void setLightPosition(const glm::vec3& position) { mLightPosition = position; }
	const glm::vec3& getLightPosition() const { return mLightPosition; }
	float getFarPlane() const { return mFarPlane; }

	// Render one face (0..5) into the cube map: binds it and clears it.
	// Returns the light space matrix (projection * view) of this face.
	glm::mat4 beginFace(int face);
	void end(int viewportWidth, int viewportHeight);

	void bind(GLuint texUnit);

private:
	PointShadowMap(const PointShadowMap& rhs);
	PointShadowMap& operator = (const PointShadowMap& rhs);

	int mSize;
	float mFarPlane;
	glm::vec3 mLightPosition;
	GLuint mFBO;
	GLuint mCubeTexture;
};
#endif //POINTSHADOWMAP_H
