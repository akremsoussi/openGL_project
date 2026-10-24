//-----------------------------------------------------------------------------
// Shadow map of a point light: depth cube map.  See PointShadowMap.h
//-----------------------------------------------------------------------------
#include "graphics/PointShadowMap.h"
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"

namespace
{
	const float NEAR_PLANE = 0.1f;

	// Look direction and up vector of each cube face (OpenGL cube map convention)
	const glm::vec3 FACE_DIRECTION[6] = {
		glm::vec3( 1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0),
		glm::vec3( 0,-1, 0), glm::vec3( 0, 0, 1), glm::vec3(0, 0,-1)
	};
	const glm::vec3 FACE_UP[6] = {
		glm::vec3(0,-1, 0), glm::vec3(0,-1, 0), glm::vec3(0, 0, 1),
		glm::vec3(0, 0,-1), glm::vec3(0,-1, 0), glm::vec3(0,-1, 0)
	};
}

//-----------------------------------------------------------------------------
// Constructor / destructor
//-----------------------------------------------------------------------------
PointShadowMap::PointShadowMap()
	: mSize(0), mFarPlane(100.0f), mLightPosition(0.0f), mFBO(0), mCubeTexture(0)
{
}

PointShadowMap::~PointShadowMap()
{
	glDeleteFramebuffers(1, &mFBO);
	glDeleteTextures(1, &mCubeTexture);
}

//-----------------------------------------------------------------------------
// 6 depth images (cube map) + a framebuffer to render into one of them
//-----------------------------------------------------------------------------
bool PointShadowMap::init(int size, float farPlane)
{
	mSize = size;
	mFarPlane = farPlane;

	glGenTextures(1, &mCubeTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mCubeTexture);
	for (int face = 0; face < 6; face++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT24, mSize, mSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, mCubeTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (!complete)
		std::cerr << "Point shadow map framebuffer is not complete" << std::endl;

	// Empty (everything lit) until something is drawn into it
	for (int face = 0; face < 6; face++)
		beginFace(face);
	end(0, 0);

	return complete;
}

//-----------------------------------------------------------------------------
// Render into one face of the cube: 90 degree view from the light
//-----------------------------------------------------------------------------
glm::mat4 PointShadowMap::beginFace(int face)
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mCubeTexture, 0);
	glViewport(0, 0, mSize, mSize);
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, NEAR_PLANE, mFarPlane);
	glm::mat4 view = glm::lookAt(mLightPosition, mLightPosition + FACE_DIRECTION[face], FACE_UP[face]);
	return projection * view;
}

//-----------------------------------------------------------------------------
// Back to the window framebuffer
//-----------------------------------------------------------------------------
void PointShadowMap::end(int viewportWidth, int viewportHeight)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (viewportWidth > 0 && viewportHeight > 0)
		glViewport(0, 0, viewportWidth, viewportHeight);
}

//-----------------------------------------------------------------------------
// Bind the cube map to a texture unit (samplerCube in the scene shader)
//-----------------------------------------------------------------------------
void PointShadowMap::bind(GLuint texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mCubeTexture);
}
