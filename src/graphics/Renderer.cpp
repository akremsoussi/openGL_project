//-----------------------------------------------------------------------------
// Draws the scene objects: each with its material (shader, lights, uniforms)
// and into the shadow maps.
//-----------------------------------------------------------------------------
#include "graphics/Renderer.h"
#include "graphics/ShadowMap.h"
#include "graphics/PointShadowMap.h"
#include "ShaderProgram.h"
#include <string>

namespace
{
	// Texture units of the shadow maps (0 = texture of the material)
	const int MAP_SHADOW_UNIT = 1;
	const int CHARACTER_SHADOW_UNIT = 2;
	const int SPOT_SHADOW_UNIT = 3;
	const int POINT_STATIC_UNIT = 4;	// 4, 5, 6
	const int POINT_DYNAMIC_UNIT = 7;	// 7, 8, 9
}

//-----------------------------------------------------------------------------
// Shadow maps used by the lit materials
//-----------------------------------------------------------------------------
void Renderer::setShadowMaps(ShadowMap* mapShadow, ShadowMap* characterShadow, bool enabled)
{
	mMapShadow = mapShadow;
	mCharacterShadow = characterShadow;
	mShadowsOn = enabled && mapShadow != nullptr && characterShadow != nullptr;
}

void Renderer::setSpotShadow(ShadowMap* spotShadow, bool on)
{
	mSpotShadow = spotShadow;
	mSpotShadowOn = on && spotShadow != nullptr;
}

void Renderer::setPointShadows(const std::vector<PointShadowMap*>& staticMaps,
                               const std::vector<PointShadowMap*>& dynamicMaps, float farPlane)
{
	mPointStatic = staticMaps;
	mPointDynamic = dynamicMaps;
	mPointFar = farPlane;
}

//-----------------------------------------------------------------------------
// Draws every visible object
//-----------------------------------------------------------------------------
void Renderer::render(Scene& scene, const LightManager& lights, const glm::mat4& view,
                      const glm::mat4& projection, const glm::vec3& cameraPosition)
{
	for (SceneObject& object : scene.objects)
	{
		if (object.visible && object.material.shader != nullptr)
			renderObject(object, lights, view, projection, cameraPosition);
	}
}

//-----------------------------------------------------------------------------
// One object: shader, matrices, lights, extra uniforms, then the model
//-----------------------------------------------------------------------------
void Renderer::renderObject(SceneObject& object, const LightManager& lights, const glm::mat4& view,
                            const glm::mat4& projection, const glm::vec3& cameraPosition)
{
	const Material& material = object.material;
	ShaderProgram& shader = *material.shader;

	// Must be called BEFORE setting uniforms
	shader.use();
	shader.setUniform("model", object.transform.getMatrix());
	shader.setUniform("view", view);
	shader.setUniform("projection", projection);
	shader.setUniform("viewPos", cameraPosition);

	if (material.receivesLighting)
	{
		lights.applyToShader(shader);
		applyShadows(shader);
	}

	for (const auto& uniform : material.floatUniforms)
		shader.setUniform(uniform.first.c_str(), uniform.second);
	for (const auto& uniform : material.vec3Uniforms)
		shader.setUniform(uniform.first.c_str(), uniform.second);
	for (const auto& uniform : material.intUniforms)
		shader.setUniform(uniform.first.c_str(), (GLint)uniform.second);

	// The textures of the model itself go on unit 0
	shader.setUniformSampler("material.diffuseMap", 0);
	object.draw(shader);
}

//-----------------------------------------------------------------------------
// All the shadow maps, each on its own texture unit.  The units are set even
// when shadows are off: two samplers of different types (2D, cube) must never
// share a unit, or OpenGL refuses to draw.
//-----------------------------------------------------------------------------
void Renderer::applyShadows(ShaderProgram& shader)
{
	shader.setUniform("shadowsOn", (GLint)mShadowsOn);

	// Moon: map + characters
	shader.setUniformSampler("shadowMap", MAP_SHADOW_UNIT);
	shader.setUniformSampler("characterShadowMap", CHARACTER_SHADOW_UNIT);
	if (mMapShadow != nullptr && mCharacterShadow != nullptr)
	{
		shader.setUniform("lightSpaceMatrix", mMapShadow->getLightSpaceMatrix());
		mMapShadow->bind(MAP_SHADOW_UNIT);
		shader.setUniform("characterLightSpaceMatrix", mCharacterShadow->getLightSpaceMatrix());
		mCharacterShadow->bind(CHARACTER_SHADOW_UNIT);
	}

	// Flashlight
	shader.setUniform("spotShadowOn", (GLint)mSpotShadowOn);
	shader.setUniformSampler("spotShadowMap", SPOT_SHADOW_UNIT);
	if (mSpotShadow != nullptr)
	{
		shader.setUniform("spotLightSpaceMatrix", mSpotShadow->getLightSpaceMatrix());
		mSpotShadow->bind(SPOT_SHADOW_UNIT);
	}

	// Lamps: static and moving cube maps of lamp i
	shader.setUniform("pointShadowFar", mPointFar);
	for (int i = 0; i < LightManager::MAX_POINT_SHADOWS; i++)
	{
		std::string index = std::to_string(i);
		shader.setUniformSampler(("pointShadowStatic" + index).c_str(), POINT_STATIC_UNIT + i);
		shader.setUniformSampler(("pointShadowDynamic" + index).c_str(), POINT_DYNAMIC_UNIT + i);
		if (i < (int)mPointStatic.size() && i < (int)mPointDynamic.size())
		{
			mPointStatic[i]->bind(POINT_STATIC_UNIT + i);
			mPointDynamic[i]->bind(POINT_DYNAMIC_UNIT + i);
		}
	}
}

//-----------------------------------------------------------------------------
// Depth of the objects of one group, seen from the light
//-----------------------------------------------------------------------------
void Renderer::renderShadows(Scene& scene, ShadowGroup group, ShadowMap& shadowMap,
                             int viewportWidth, int viewportHeight)
{
	shadowMap.begin();
	for (SceneObject& object : scene.objects)
	{
		if (!object.visible || object.shadow != group || object.shadowShader == nullptr)
			continue;

		ShaderProgram& shader = *object.shadowShader;
		shader.use();
		shader.setUniform("lightSpaceMatrix", shadowMap.getLightSpaceMatrix());
		shader.setUniform("model", object.transform.getMatrix());
		shader.setUniformSampler("material.diffuseMap", 0);	// transparent texels cast no shadow
		object.draw(shader);
	}
	shadowMap.end(viewportWidth, viewportHeight);
}

//-----------------------------------------------------------------------------
// Depth of every shadow casting object, seen from the flashlight
//-----------------------------------------------------------------------------
void Renderer::renderSpotShadows(Scene& scene, ShadowMap& shadowMap, int viewportWidth, int viewportHeight)
{
	shadowMap.begin();
	for (SceneObject& object : scene.objects)
	{
		if (!object.visible || !object.castsShadows || object.shadowShader == nullptr)
			continue;

		ShaderProgram& shader = *object.shadowShader;
		shader.use();
		shader.setUniform("lightSpaceMatrix", shadowMap.getLightSpaceMatrix());
		shader.setUniform("model", object.transform.getMatrix());
		shader.setUniformSampler("material.diffuseMap", 0);
		object.draw(shader);
	}
	shadowMap.end(viewportWidth, viewportHeight);
}

//-----------------------------------------------------------------------------
// Distance of the objects to a lamp, in the 6 faces of its cube shadow map
//-----------------------------------------------------------------------------
void Renderer::renderPointShadows(Scene& scene, PointShadowMap& shadowMap, bool animated, float range,
                                  int viewportWidth, int viewportHeight)
{
	const glm::vec3& lightPos = shadowMap.getLightPosition();

	for (int face = 0; face < 6; face++)
	{
		glm::mat4 lightSpaceMatrix = shadowMap.beginFace(face);

		for (SceneObject& object : scene.objects)
		{
			if (!object.visible || !object.castsShadows || object.pointShadowShader == nullptr ||
				object.animated != animated)
				continue;

			// Moving characters far from the lamp: their shadow would be tiny and dark
			if (animated && glm::length(object.transform.position - lightPos) > range)
				continue;

			ShaderProgram& shader = *object.pointShadowShader;
			shader.use();
			shader.setUniform("lightSpaceMatrix", lightSpaceMatrix);
			shader.setUniform("model", object.transform.getMatrix());
			shader.setUniform("lightPos", lightPos);
			shader.setUniform("farPlane", shadowMap.getFarPlane());
			shader.setUniformSampler("material.diffuseMap", 0);
			object.draw(shader);
		}
	}
	shadowMap.end(viewportWidth, viewportHeight);
}
