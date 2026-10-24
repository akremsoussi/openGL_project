//-----------------------------------------------------------------------------
// HDR rendering with bloom.  See HDRPipeline.h
//-----------------------------------------------------------------------------
#include "graphics/HDRPipeline.h"
#include <algorithm>
#include <iostream>

namespace
{
	const char* POST_VERTEX_SHADER = "resources/shaders/post.vert";
	const int BLUR_PASSES = 8;		// 4 horizontal + 4 vertical

	bool loadPostShader(ShaderProgram& shader, const char* fragmentFile)
	{
		shader.loadShaders(POST_VERTEX_SHADER, fragmentFile);

		GLint linked = GL_FALSE;
		glGetProgramiv(shader.getProgram(), GL_LINK_STATUS, &linked);
		return linked == GL_TRUE;
	}

	// RGBA 16 bit float texture: colors are not clamped to 1
	GLuint createFloatTexture(int width, int height)
	{
		GLuint texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
		return texture;
	}
}

//-----------------------------------------------------------------------------
// Constructor / destructor
//-----------------------------------------------------------------------------
HDRPipeline::HDRPipeline()
	: mVAO(0), mSceneFBO(0), mSceneTexture(0), mDepth(0), mWidth(0), mHeight(0)
{
	mBlurFBO[0] = mBlurFBO[1] = 0;
	mBlurTexture[0] = mBlurTexture[1] = 0;
	mViewport[0] = mViewport[1] = mViewport[2] = mViewport[3] = 0;
}

HDRPipeline::~HDRPipeline()
{
	deleteTargets();
	glDeleteVertexArrays(1, &mVAO);
}

//-----------------------------------------------------------------------------
// Loads the post-processing shaders and creates the images
//-----------------------------------------------------------------------------
bool HDRPipeline::init(int width, int height)
{
	if (!loadPostShader(mBright, "resources/shaders/bright.frag") ||
		!loadPostShader(mBlur, "resources/shaders/blur.frag") ||
		!loadPostShader(mComposite, "resources/shaders/composite.frag"))
	{
		std::cerr << "HDR shaders failed" << std::endl;
		return false;
	}

	glGenVertexArrays(1, &mVAO);
	return createTargets(width, height);
}

//-----------------------------------------------------------------------------
// Scene image (full size, with depth) and the two blur images (half size).
// Recreated when the window size changes.
//-----------------------------------------------------------------------------
bool HDRPipeline::createTargets(int width, int height)
{
	if (width <= 0 || height <= 0)
		return false;
	if (width == mWidth && height == mHeight)
		return true;

	deleteTargets();

	// Scene: color (float) + depth
	mSceneTexture = createFloatTexture(width, height);
	glGenRenderbuffers(1, &mDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &mSceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mSceneTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
	bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	// Blur: half resolution (cheaper and blurs twice as far)
	glGenFramebuffers(2, mBlurFBO);
	for (int i = 0; i < 2; i++)
	{
		mBlurTexture[i] = createFloatTexture(std::max(1, width / 2), std::max(1, height / 2));
		glBindFramebuffer(GL_FRAMEBUFFER, mBlurFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBlurTexture[i], 0);
		complete = complete && (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (!complete)
	{
		std::cerr << "HDR framebuffers are not complete" << std::endl;
		return false;
	}

	mWidth = width;
	mHeight = height;
	return true;
}

void HDRPipeline::deleteTargets()
{
	glDeleteFramebuffers(1, &mSceneFBO);
	glDeleteTextures(1, &mSceneTexture);
	glDeleteRenderbuffers(1, &mDepth);
	glDeleteFramebuffers(2, mBlurFBO);
	glDeleteTextures(2, mBlurTexture);
	mSceneFBO = mSceneTexture = mDepth = 0;
	mBlurFBO[0] = mBlurFBO[1] = mBlurTexture[0] = mBlurTexture[1] = 0;
	mWidth = mHeight = 0;
}

//-----------------------------------------------------------------------------
// Following draws go into the HDR scene image
//-----------------------------------------------------------------------------
bool HDRPipeline::begin(int width, int height)
{
	if (mVAO == 0 || !createTargets(width, height))
		return false;

	glGetIntegerv(GL_VIEWPORT, mViewport);
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return true;
}

//-----------------------------------------------------------------------------
// Bright pass, blur, then the final image on the window
//-----------------------------------------------------------------------------
void HDRPipeline::finish(bool bloom)
{
	// Full screen triangles: no depth test, filled even in wireframe mode
	GLint polygonMode[2];
	glGetIntegerv(GL_POLYGON_MODE, polygonMode);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(mVAO);
	glActiveTexture(GL_TEXTURE0);

	if (bloom)
	{
		// 1. Bright parts of the scene -> blur image 0 (half size)
		glViewport(0, 0, std::max(1, mWidth / 2), std::max(1, mHeight / 2));
		glBindFramebuffer(GL_FRAMEBUFFER, mBlurFBO[0]);
		mBright.use();
		glBindTexture(GL_TEXTURE_2D, mSceneTexture);
		mBright.setUniform("source", (GLint)0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// 2. Gaussian blur, one direction per pass: 0 -> 1 -> 0 ...
		mBlur.use();
		mBlur.setUniform("source", (GLint)0);
		for (int pass = 0; pass < BLUR_PASSES; pass++)
		{
			int source = pass % 2;
			int target = 1 - source;
			glBindFramebuffer(GL_FRAMEBUFFER, mBlurFBO[target]);
			glBindTexture(GL_TEXTURE_2D, mBlurTexture[source]);
			mBlur.setUniform("axis", (pass % 2 == 0) ? glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f));
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}

	// 3. Scene + bloom -> window
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
	mComposite.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mSceneTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mBlurTexture[0]);	// even number of passes: result in 0
	mComposite.setUniform("scene", (GLint)0);
	mComposite.setUniform("bloom", (GLint)1);
	mComposite.setUniform("bloomEnabled", (GLint)bloom);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Restore the state for the next frame
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
	glEnable(GL_DEPTH_TEST);
}
