//-----------------------------------------------------------------------------
// Shadow map: depth texture rendered from the point of view of a directional
// light, then used by the scene shader to know which fragments are in shadow.
//-----------------------------------------------------------------------------
#include "graphics/ShadowMap.h"
#include <iostream>
#include <cmath>
#include "glm/gtc/matrix_transform.hpp"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
ShadowMap::ShadowMap()
	: mSize(0), mFBO(0), mDepthTexture(0), mLightSpaceMatrix(1.0f)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ShadowMap::~ShadowMap()
{
	glDeleteFramebuffers(1, &mFBO);
	glDeleteTextures(1, &mDepthTexture);
}

//-----------------------------------------------------------------------------
// Creates the depth texture (size x size) and the framebuffer that renders in it
//-----------------------------------------------------------------------------
bool ShadowMap::init(int size)
{
	mSize = size;

	glGenTextures(1, &mDepthTexture);
	glBindTexture(GL_TEXTURE_2D, mDepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mSize, mSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Linear filtering + compare mode: the GPU returns the fraction of the
	// 4 nearest texels that are lit (smoother shadow edges, sampler2DShadow)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Outside of the shadow map everything is lit
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTexture, 0);

	// Depth only: no color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (!complete)
		std::cerr << "Shadow map framebuffer is not complete" << std::endl;

	return complete;
}

//-----------------------------------------------------------------------------
// Orthographic projection (directional light = parallel rays) looking along
// lightDirection and fitted around the 8 corners of the box.
//-----------------------------------------------------------------------------
void ShadowMap::setLight(const glm::vec3& lightDirection, const glm::vec3& boxMin, const glm::vec3& boxMax)
{
	glm::vec3 dir = glm::normalize(lightDirection);
	glm::vec3 center = (boxMin + boxMax) * 0.5f;
	float radius = glm::length(boxMax - boxMin) * 0.5f;

	// Avoid an up vector parallel to the light direction
	glm::vec3 up = (fabs(dir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 lightView = glm::lookAt(center - dir * radius, center, up);

	// Box corners in light space -> bounds of the orthographic projection
	glm::vec3 minLS(1e30f), maxLS(-1e30f);
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 corner((i & 1) ? boxMax.x : boxMin.x,
		                 (i & 2) ? boxMax.y : boxMin.y,
		                 (i & 4) ? boxMax.z : boxMin.z);
		glm::vec3 p = glm::vec3(lightView * glm::vec4(corner, 1.0f));
		minLS = glm::min(minLS, p);
		maxLS = glm::max(maxLS, p);
	}

	// The camera looks down -Z in view space: near/far are -maxZ/-minZ
	glm::mat4 lightProjection = glm::ortho(minLS.x, maxLS.x, minLS.y, maxLS.y, -maxLS.z, -minLS.z);

	mLightSpaceMatrix = lightProjection * lightView;
}

//-----------------------------------------------------------------------------
// Spot light: the rays leave one point, so the projection is a perspective
// (like a camera placed at the light)
//-----------------------------------------------------------------------------
void ShadowMap::setSpotLight(const glm::vec3& position, const glm::vec3& direction, float fovDegrees, float nearPlane, float farPlane)
{
	glm::vec3 dir = glm::normalize(direction);
	glm::vec3 up = (fabs(dir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 lightView = glm::lookAt(position, position + dir, up);
	glm::mat4 lightProjection = glm::perspective(glm::radians(fovDegrees), 1.0f, nearPlane, farPlane);

	mLightSpaceMatrix = lightProjection * lightView;
}

//-----------------------------------------------------------------------------
// Start rendering the depth of the scene into the shadow map
//-----------------------------------------------------------------------------
void ShadowMap::begin()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(0, 0, mSize, mSize);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Push the depth a little to avoid "shadow acne" (surfaces shadowing themselves)
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);
}

//-----------------------------------------------------------------------------
// Back to the window framebuffer
//-----------------------------------------------------------------------------
void ShadowMap::end(int viewportWidth, int viewportHeight)
{
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, viewportWidth, viewportHeight);
}

//-----------------------------------------------------------------------------
// Bind the depth texture to a texture unit (for the scene shader)
//-----------------------------------------------------------------------------
void ShadowMap::bind(GLuint texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, mDepthTexture);
}
