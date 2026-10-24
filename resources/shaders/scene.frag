//-----------------------------------------------------------------------------
// Fragment shader for the scene: directional (sun), point and spot lights
//
// Based on IOGL-homework/shaders/lighting_dir_point_spot.frag with:
//  - the material colors read from the .mtl file (Kd used when there is no texture)
//  - transparent texels (ex: leaves) discarded
//  - ambient light coming from the sun
//  - a variable number of point lights (pointLightCount, set by LightManager)
//  - shadows of the directional light: one shadow map for the (static) scene
//    and a small one for the animated characters, updated every frame
//  - shadows of the flashlight (perspective shadow map, every frame)
//  - shadows of the first 3 point lights (lamps): cube maps storing the
//    distance to the light, a static one and one for the moving characters
//-----------------------------------------------------------------------------
#version 330 core

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    sampler2D diffuseMap;
    int hasDiffuseMap;
    vec3 specular;
    float shininess;
};

struct DirectionalLight
{
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float exponent;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;
	float cosInnerCone;
	float cosOuterCone;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	int on;

	float constant;
	float linear;
	float exponent;
};


in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

#define MAX_POINT_LIGHTS 8	// must match LightManager::MAX_POINT_LIGHTS

uniform DirectionalLight sunLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int pointLightCount;		// lights used in pointLights
uniform SpotLight spotLight;
uniform Material material;
uniform vec3 viewPos;

uniform sampler2DShadow shadowMap;	// depth of the scene seen from the directional light
uniform mat4 lightSpaceMatrix;		// world -> shadow map
uniform sampler2DShadow characterShadowMap;	// depth of the animated characters only
uniform mat4 characterLightSpaceMatrix;
uniform int shadowsOn;
uniform int winterMap; // Enabled only for the map, never the characters.

// Flashlight shadows
uniform sampler2DShadow spotShadowMap;
uniform mat4 spotLightSpaceMatrix;
uniform int spotShadowOn;

// Lamp shadows: pointLights[0..pointShadowCount-1] have cube shadow maps.
// One sampler per map (GLSL 3.30 cannot pick a sampler in an array with a variable).
#define MAX_POINT_SHADOWS 3	// must match LightManager::MAX_POINT_SHADOWS
uniform samplerCube pointShadowStatic0;		// map and still objects
uniform samplerCube pointShadowStatic1;
uniform samplerCube pointShadowStatic2;
uniform samplerCube pointShadowDynamic0;	// animated characters
uniform samplerCube pointShadowDynamic1;
uniform samplerCube pointShadowDynamic2;
uniform int pointShadowCount;
uniform float pointShadowFar;				// distance stored as distance / far

out vec4 frag_color;

float calcShadow(sampler2DShadow map, mat4 toShadowMap, vec3 normal, float normalOffset);
float calcPointShadow(samplerCube staticMap, samplerCube dynamicMap, vec3 lightPos, vec3 normal);
vec3 calcDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 baseColor);
vec3 calcPointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);
vec3 calcSpotLightColor(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);

// Continuous world-space variation keeps the snow independent of UV seams.
float snowNoise(vec3 p)
{
    vec3 cell = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n = dot(cell, vec3(1.0, 57.0, 113.0));
    return mix(mix(mix(fract(sin(n) * 43758.5453), fract(sin(n + 1.0) * 43758.5453), f.x),
                   mix(fract(sin(n + 57.0) * 43758.5453), fract(sin(n + 58.0) * 43758.5453), f.x), f.y),
               mix(mix(fract(sin(n + 113.0) * 43758.5453), fract(sin(n + 114.0) * 43758.5453), f.x),
                   mix(fract(sin(n + 170.0) * 43758.5453), fract(sin(n + 171.0) * 43758.5453), f.x), f.y), f.z);
}

//-----------------------------------------------------------------------------------------------
// Main Shader Entry
//-----------------------------------------------------------------------------------------------
void main()
{
	// Surface color: texture if the material has one, else the Kd color
	vec4 texel = vec4(material.diffuse, 1.0f);
	if (material.hasDiffuseMap == 1)
		texel = texture(material.diffuseMap, TexCoord);

	// Cut out transparent parts (leaves, fences...)
	if (texel.a < 0.5f)
		discard;

	vec3 baseColor = texel.rgb;

	vec3 normal = normalize(Normal);
	if (winterMap == 1)
	{
		// Use the original world normal before the camera-facing lighting flip.
		float drift = snowNoise(FragPos * 0.24);
		float grain = snowNoise(FragPos * 8.0);
		float green = smoothstep(0.015, 0.10, baseColor.g - max(baseColor.r, baseColor.b));
		float saturation = max(baseColor.r, max(baseColor.g, baseColor.b))
		                 - min(baseColor.r, min(baseColor.g, baseColor.b));
		// Saturated red/blue crystals and gold details retain their identity.
		float stone = 1.0 - smoothstep(0.16, 0.38, saturation);
		float top = smoothstep(0.25 + 0.20 * drift, 0.78, normal.y);
		float coverage = max(top * stone * 0.94, green * (0.72 + 0.22 * drift));
		float detail = dot(baseColor, vec3(0.2126, 0.7152, 0.0722));
		vec3 snow = vec3(0.78, 0.87, 0.95) * (0.80 + 0.16 * detail + 0.04 * grain);
		baseColor = mix(baseColor * vec3(0.90, 0.96, 1.04), snow, coverage);
	}
	vec3 viewDir = normalize(viewPos - FragPos);

	// Planes are often single sided in exported models: light the side we see
	if (dot(normal, viewDir) < 0.0f)
		normal = -normal;

    // Ambient ----------------------------------------------------------------------------------
	vec3 ambient = sunLight.ambient * material.ambient * baseColor;
	vec3 outColor = vec3(0.0f);

	float lit = 1.0f;
	if (shadowsOn == 1)
		lit = min(calcShadow(shadowMap, lightSpaceMatrix, normal, 0.08f),
		          calcShadow(characterShadowMap, characterLightSpaceMatrix, normal, 0.02f));

	outColor += lit * calcDirectionalLightColor(sunLight, normal, viewDir, baseColor);

	// Lamp shadows (the first lights have a cube shadow map)
	float pointLit[MAX_POINT_LIGHTS];
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		pointLit[i] = 1.0f;
	if (shadowsOn == 1)
	{
		if (pointShadowCount > 0)
			pointLit[0] = calcPointShadow(pointShadowStatic0, pointShadowDynamic0, pointLights[0].position, normal);
		if (pointShadowCount > 1)
			pointLit[1] = calcPointShadow(pointShadowStatic1, pointShadowDynamic1, pointLights[1].position, normal);
		if (pointShadowCount > 2)
			pointLit[2] = calcPointShadow(pointShadowStatic2, pointShadowDynamic2, pointLights[2].position, normal);
	}

   for(int i = 0; i < pointLightCount; i++)
        outColor += pointLit[i] * calcPointLightColor(pointLights[i], normal, FragPos, viewDir, baseColor);

	// If the light isn't on then just return 0 for diffuse and specular colors
	if (spotLight.on == 1)
	{
		float spotLit = 1.0f;
		if (shadowsOn == 1 && spotShadowOn == 1)
			spotLit = calcShadow(spotShadowMap, spotLightSpaceMatrix, normal, 0.03f);
		outColor += spotLit * calcSpotLightColor(spotLight, normal, FragPos, viewDir, baseColor);
	}

	frag_color = vec4(ambient + outColor, 1.0f);
}

//-----------------------------------------------------------------------------------------------
// Returns how much this fragment receives the directional light:
// 1 = fully lit, 0 = fully in shadow (soft edges with 3x3 PCF)
//-----------------------------------------------------------------------------------------------
float calcShadow(sampler2DShadow map, mat4 toShadowMap, vec3 normal, float normalOffset)
{
	// Move the point a little along its normal to avoid "shadow acne"
	vec4 lightSpacePos = toShadowMap * vec4(FragPos + normal * normalOffset, 1.0f);
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w * 0.5f + 0.5f;	// [-1,1] -> [0,1]

	// Beyond the far plane of the light: compare with the farthest depth, so
	// what is behind an object (seen from the light) is still in its shadow
	projCoords.z = min(projCoords.z, 1.0f);

	// Percentage Closer Filtering: average of 9 depth comparisons around the point
	vec2 texelSize = 1.0f / vec2(textureSize(map, 0));
	float lit = 0.0f;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
			lit += texture(map, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - 0.0005f));
	}

	return lit / 9.0f;
}

//-----------------------------------------------------------------------------------------------
// How much this fragment receives a lamp (point light), with its cube shadow
// maps: 1 = lit, 0 = in shadow.  The cube map is read in the direction
// light -> fragment; it gives the distance to the closest object in that
// direction.  If the fragment is farther, something is between it and the lamp.
//-----------------------------------------------------------------------------------------------
float calcPointShadow(samplerCube staticMap, samplerCube dynamicMap, vec3 lightPos, vec3 normal)
{
	// Move the point a little along its normal to avoid "shadow acne"
	vec3 lightToFrag = FragPos + normal * 0.1f - lightPos;
	float dist = length(lightToFrag);
	if (dist > pointShadowFar)
		return 1.0f;

	// Soft edges: 5 directions around the exact one (PCF), more blur far away
	vec3 dir = lightToFrag / dist;
	vec3 side = normalize(cross(dir, abs(dir.y) < 0.99f ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f)));
	vec3 up = cross(dir, side);
	float spread = 0.004f + 0.0002f * dist;
	vec3 offsets[5] = vec3[](vec3(0.0f), side * spread, -side * spread, up * spread, -up * spread);

	float bias = 0.05f + 0.002f * dist;
	float lit = 0.0f;
	for (int i = 0; i < 5; i++)
	{
		vec3 d = dir + offsets[i];
		float closest = min(texture(staticMap, d).r, texture(dynamicMap, d).r) * pointShadowFar;
		lit += (dist - bias > closest) ? 0.0f : 1.0f;
	}
	return lit / 5.0f;
}

//-----------------------------------------------------------------------------------------------
// Calculate the direction light effect and return the resulting
// diffuse and specular color summation
//-----------------------------------------------------------------------------------------------
vec3 calcDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 baseColor)
{
	vec3 lightDir = normalize(-light.direction);  // negate => Must be a direction from fragment towards the light

	// Diffuse ------------------------------------------------------------------------- --------
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * NdotL * baseColor;

     // Specular - Blinn-Phong ------------------------------------------------------------------
	vec3 halfDir = normalize(lightDir + viewDir);
	float NDotH = max(dot(normal, halfDir), 0.0f);
	vec3 specular = light.specular * material.specular * pow(NDotH, material.shininess);

	return (diffuse + specular);
}

//-----------------------------------------------------------------------------------------------
// Calculate the point light effect and return the resulting diffuse and specular color summation
//-----------------------------------------------------------------------------------------------
vec3 calcPointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
	vec3 lightDir = normalize(light.position - fragPos);

	// Diffuse ----------------------------------------------------------------------------------
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * NdotL * baseColor;

     // Specular - Blinn-Phong ------------------------------------------------------------------
	vec3 halfDir = normalize(lightDir + viewDir);
	float NDotH = max(dot(normal, halfDir), 0.0f);
	vec3 specular = light.specular * material.specular * pow(NDotH, material.shininess);

	// Attenuation using Kc, Kl, Kq -------------------------------------------------------------
	float d = length(light.position - fragPos);
	float attenuation = 1.0f / (light.constant + light.linear * d + light.exponent * (d * d));

	diffuse *= attenuation;
	specular *= attenuation;

	return (diffuse + specular);
}

//------------------------------------------------------------------------------------------------
// Calculate the spotlight effect and return the resulting // diffuse and specular color summation
//------------------------------------------------------------------------------------------------
vec3 calcSpotLightColor(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
	vec3 lightDir = normalize(light.position - fragPos);
	vec3 spotDir  = normalize(light.direction);

	float cosDir = dot(-lightDir, spotDir);  // angle between the lights direction vector and spotlights direction vector
	float spotIntensity = smoothstep(light.cosOuterCone, light.cosInnerCone, cosDir);

	// Diffuse ----------------------------------------------------------------------------------
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * NdotL * baseColor;

     // Specular - Blinn-Phong ------------------------------------------------------------------
	vec3 halfDir = normalize(lightDir + viewDir);
	float NDotH = max(dot(normal, halfDir), 0.0f);
	vec3 specular = light.specular * material.specular * pow(NDotH, material.shininess);

	// Attenuation using Kc, Kl, Kq -------------------------------------------------------------
	float d = length(light.position - fragPos);
	float attenuation = 1.0f / (light.constant + light.linear * d + light.exponent * (d * d));

	diffuse *= attenuation * spotIntensity;
	specular *= attenuation * spotIntensity;

	return (diffuse + specular);
}
