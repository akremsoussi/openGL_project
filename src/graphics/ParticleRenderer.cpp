//-----------------------------------------------------------------------------
// Draws all the particles in one call (instancing)
//-----------------------------------------------------------------------------
#include "graphics/ParticleRenderer.h"
#include <algorithm>
#include <cstddef>

//-----------------------------------------------------------------------------
// Constructor / destructor
//-----------------------------------------------------------------------------
ParticleRenderer::ParticleRenderer()
	: mVAO(0), mBuffer(0)
{
}

ParticleRenderer::~ParticleRenderer()
{
	glDeleteBuffers(1, &mBuffer);
	glDeleteVertexArrays(1, &mVAO);
}

//-----------------------------------------------------------------------------
// Shader + a buffer with one Instance per particle.  The 4 corners of each
// square are made in the vertex shader from gl_VertexID.
//-----------------------------------------------------------------------------
bool ParticleRenderer::init()
{
	mShader.loadShaders("resources/shaders/particle.vert", "resources/shaders/particle.frag");

	GLint linked = GL_FALSE;
	glGetProgramiv(mShader.getProgram(), GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE)
		return false;

	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mBuffer);
	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mBuffer);

	// Divisor 1: these attributes change once per square (instance), not per vertex
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), (GLvoid*)0);	// position + size
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), (GLvoid*)offsetof(Instance, opacity));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return true;
}

//-----------------------------------------------------------------------------
// Uploads the particles and draws them all at once
//-----------------------------------------------------------------------------
void ParticleRenderer::render(const std::vector<Particle>& particles, const glm::mat4& view, const glm::mat4& projection)
{
	if (mVAO == 0 || particles.empty())
		return;

	// Fade in at birth and out before death
	mInstances.clear();
	for (const Particle& p : particles)
	{
		float life = p.age / p.lifetime;
		float opacity = std::min(1.0f, life * 5.0f) * std::min(1.0f, (1.0f - life) * 5.0f);
		Instance instance = { p.position.x, p.position.y, p.position.z, p.size, opacity };
		mInstances.push_back(instance);
	}

	// Transparent: hidden by the scene (depth test) but they do not hide each
	// other (no depth write).  Normal alpha blending (not additive) so white
	// flakes stay visible over the bright snowy ground and the day sky.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	mShader.use();
	mShader.setUniform("view", view);
	mShader.setUniform("projection", projection);

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
	glBufferData(GL_ARRAY_BUFFER, mInstances.size() * sizeof(Instance), &mInstances[0], GL_STREAM_DRAW);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)mInstances.size());

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}
