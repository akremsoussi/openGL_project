//-----------------------------------------------------------------------------
// All the lights of the scene and how they are sent to scene.frag:
//  - one directional light (moon by night, sun by day)
//  - point lights (lamps): the brightest MAX_POINT_LIGHTS are used
//  - one spot light (flashlight held by the camera)
//-----------------------------------------------------------------------------
#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include <vector>
#include "glm/glm.hpp"

class ShaderProgram;

struct DirectionalLight
{
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);	// direction the light travels
	glm::vec3 ambient = glm::vec3(0.1f);
	glm::vec3 diffuse = glm::vec3(1.0f);
	glm::vec3 specular = glm::vec3(1.0f);
};

struct PointLight
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 1.0f;

	// Attenuation 1 / (constant + linear * d + quadratic * d^2)
	float constant = 1.0f;
	float linear = 0.045f;
	float quadratic = 0.0075f;

	bool castsShadows = false;	// has a cube shadow map (at most MAX_POINT_SHADOWS lights)
};

struct SpotLight
{
	bool on = false;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
	float innerConeDegrees = 15.0f;
	float outerConeDegrees = 20.0f;
	glm::vec3 color = glm::vec3(0.8f);
	float constant = 1.0f;
	float linear = 0.07f;
	float quadratic = 0.017f;
};

class LightManager
{
public:
	// Must match MAX_POINT_LIGHTS and MAX_POINT_SHADOWS in scene.frag
	static const int MAX_POINT_LIGHTS = 8;
	static const int MAX_POINT_SHADOWS = 3;

	void setDirectionalLight(const DirectionalLight& light) { mDirectionalLight = light; }
	const DirectionalLight& getDirectionalLight() const { return mDirectionalLight; }

	void addPointLight(const PointLight& light) { mPointLights.push_back(light); }
	std::vector<PointLight>& getPointLights() { return mPointLights; }

	SpotLight& getSpotLight() { return mSpotLight; }

	// Lights sent to the shader, at most MAX_POINT_LIGHTS: first the shadow
	// casting ones (in the order they were added, like their shadow maps),
	// then the others, brightest first
	std::vector<PointLight> shaderPointLights() const;
	int shadowCastingCount() const;		// lights with a shadow map (first in the list)

	// Sets the sunLight, pointLights[] and spotLight uniforms of scene.frag
	void applyToShader(ShaderProgram& shader) const;

private:
	DirectionalLight mDirectionalLight;
	std::vector<PointLight> mPointLights;
	SpotLight mSpotLight;
};
#endif //LIGHTMANAGER_H
