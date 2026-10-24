//-----------------------------------------------------------------------------
// HDR rendering with bloom (glow around bright lights):
//  1. the scene is drawn into a floating point image (colors can go above 1)
//  2. bright pass: keep only what is brighter than white   (bright.frag)
//  3. blur it several times, horizontally then vertically  (blur.frag)
//  4. add the blur to the scene and compress the highlights (composite.frag)
// Steps 2-4 draw one full screen triangle each (post.vert).
//-----------------------------------------------------------------------------
#ifndef HDRPIPELINE_H
#define HDRPIPELINE_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "ShaderProgram.h"

class HDRPipeline
{
public:
	 HDRPipeline();
	~HDRPipeline();

	bool init(int width, int height);

	// Draw the scene between begin() and finish().  begin() clears with the
	// current clear color.
	bool begin(int width, int height);
	void finish(bool bloom = true);

private:
	HDRPipeline(const HDRPipeline& rhs);
	HDRPipeline& operator = (const HDRPipeline& rhs);

	bool createTargets(int width, int height);
	void deleteTargets();

	ShaderProgram mBright, mBlur, mComposite;
	GLuint mVAO;					// empty: post.vert makes the triangle from gl_VertexID
	GLuint mSceneFBO, mSceneTexture, mDepth;
	GLuint mBlurFBO[2], mBlurTexture[2];	// half resolution, ping-pong
	int mWidth, mHeight;
	GLint mViewport[4];
};
#endif //HDRPIPELINE_H
