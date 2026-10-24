//-----------------------------------------------------------------------------
// The main light made visible: moon / sun disk in the sky
//-----------------------------------------------------------------------------
#include "graphics/SkyLight.h"

namespace
{
	const float DISTANCE = 1000.0f;		// far, but in front of the far plane (2000)
	const float RADIUS = 30.0f;			// disk radius at that distance (~1.7 degrees)
}

//-----------------------------------------------------------------------------
// Constructor / destructor
//-----------------------------------------------------------------------------
SkyLight::SkyLight()
	: mVAO(0)
{
}

SkyLight::~SkyLight()
{
	glDeleteVertexArrays(1, &mVAO);
}

bool SkyLight::init()
{
	mShader.loadShaders("resources/shaders/sky_light.vert", "resources/shaders/sky_light.frag");
	glGenVertexArrays(1, &mVAO);

	GLint linked = GL_FALSE;
	glGetProgramiv(mShader.getProgram(), GL_LINK_STATUS, &linked);
	return linked == GL_TRUE;
}

//-----------------------------------------------------------------------------
// Draws the disk behind the scene objects (depth test) without hiding the
// rest of the sky (no depth write), added to the sky color (glow)
//-----------------------------------------------------------------------------
void SkyLight::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition,
                      const glm::vec3& lightDirection, const glm::vec3& color)
{
	if (mVAO == 0)
		return;

	// The light comes FROM the opposite of its travel direction
	glm::vec3 center = cameraPosition - glm::normalize(lightDirection) * DISTANCE;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	mShader.use();
	mShader.setUniform("view", view);
	mShader.setUniform("projection", projection);
	mShader.setUniform("center", center);
	mShader.setUniform("radius", RADIUS);
	mShader.setUniform("color", color);

	glBindVertexArray(mVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}
