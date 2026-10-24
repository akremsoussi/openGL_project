//-----------------------------------------------------------------------------
// The main light made visible: the moon (or the sun by day) drawn as a glowing
// disk in the sky, exactly in the direction its light comes from, so it matches
// the lighting and the shadows.  One square facing the camera, far away.
//-----------------------------------------------------------------------------
#ifndef SKYLIGHT_H
#define SKYLIGHT_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "ShaderProgram.h"

class SkyLight
{
public:
	 SkyLight();
	~SkyLight();

	bool init();

	// lightDirection: direction the light travels (as in DirectionalLight).
	// color can be brighter than 1: the bloom then makes it glow.
	void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition,
	            const glm::vec3& lightDirection, const glm::vec3& color);

private:
	SkyLight(const SkyLight& rhs);
	SkyLight& operator = (const SkyLight& rhs);

	ShaderProgram mShader;
	GLuint mVAO;		// empty: the corners come from gl_VertexID
};
#endif //SKYLIGHT_H
