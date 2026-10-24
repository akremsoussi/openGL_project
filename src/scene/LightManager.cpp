//-----------------------------------------------------------------------------
// All the lights of the scene and how they are sent to scene.frag
//-----------------------------------------------------------------------------
#include "scene/LightManager.h"
#include "ShaderProgram.h"
#include <algorithm>
#include <string>

//-----------------------------------------------------------------------------
// The shader has room for MAX_POINT_LIGHTS lights.  The shadow casting ones go
// first, in their order (light i uses shadow map i), then the brightest others
// (lights of equal intensity keep their order).
//-----------------------------------------------------------------------------
std::vector<PointLight> LightManager::shaderPointLights() const
{
	std::vector<PointLight> casters, others;
	for (const PointLight& light : mPointLights)
	{
		if (light.castsShadows && (int)casters.size() < MAX_POINT_SHADOWS)
			casters.push_back(light);
		else if (light.intensity > 0.0f)
			others.push_back(light);
	}

	std::stable_sort(others.begin(), others.end(),
		[](const PointLight& a, const PointLight& b) { return a.intensity > b.intensity; });

	std::vector<PointLight> selected = casters;
	selected.insert(selected.end(), others.begin(), others.end());
	if (selected.size() > (size_t)MAX_POINT_LIGHTS)
		selected.resize(MAX_POINT_LIGHTS);

	return selected;
}

//-----------------------------------------------------------------------------
// Number of lights with a shadow map
//-----------------------------------------------------------------------------
int LightManager::shadowCastingCount() const
{
	int count = 0;
	for (const PointLight& light : mPointLights)
	{
		if (light.castsShadows)
			count++;
	}
	return std::min(count, MAX_POINT_SHADOWS);
}

//-----------------------------------------------------------------------------
// Sets the light uniforms.  The shader must be active.
//-----------------------------------------------------------------------------
void LightManager::applyToShader(ShaderProgram& shader) const
{
	// Directional light
	shader.setUniform("sunLight.direction", mDirectionalLight.direction);
	shader.setUniform("sunLight.ambient", mDirectionalLight.ambient);
	shader.setUniform("sunLight.diffuse", mDirectionalLight.diffuse);
	shader.setUniform("sunLight.specular", mDirectionalLight.specular);

	// Point lights
	std::vector<PointLight> points = shaderPointLights();
	shader.setUniform("pointLightCount", (GLint)points.size());
	shader.setUniform("pointShadowCount", (GLint)shadowCastingCount());
	for (size_t i = 0; i < points.size(); i++)
	{
		const PointLight& light = points[i];
		std::string name = "pointLights[" + std::to_string(i) + "].";

		shader.setUniform((name + "ambient").c_str(), glm::vec3(0.0f));
		shader.setUniform((name + "diffuse").c_str(), light.color * light.intensity);
		shader.setUniform((name + "specular").c_str(), light.color);
		shader.setUniform((name + "position").c_str(), light.position);
		shader.setUniform((name + "constant").c_str(), light.constant);
		shader.setUniform((name + "linear").c_str(), light.linear);
		shader.setUniform((name + "exponent").c_str(), light.quadratic);
	}

	// Spot light (flashlight)
	shader.setUniform("spotLight.on", (GLint)mSpotLight.on);
	shader.setUniform("spotLight.ambient", glm::vec3(0.1f));
	shader.setUniform("spotLight.diffuse", mSpotLight.color);
	shader.setUniform("spotLight.specular", glm::vec3(1.0f));
	shader.setUniform("spotLight.position", mSpotLight.position);
	shader.setUniform("spotLight.direction", mSpotLight.direction);
	shader.setUniform("spotLight.cosInnerCone", glm::cos(glm::radians(mSpotLight.innerConeDegrees)));
	shader.setUniform("spotLight.cosOuterCone", glm::cos(glm::radians(mSpotLight.outerConeDegrees)));
	shader.setUniform("spotLight.constant", mSpotLight.constant);
	shader.setUniform("spotLight.linear", mSpotLight.linear);
	shader.setUniform("spotLight.exponent", mSpotLight.quadratic);
}
