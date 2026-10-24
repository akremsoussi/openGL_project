//-----------------------------------------------------------------------------
// Builds the ARAM scene: loads the resources, places the objects and the lights
//-----------------------------------------------------------------------------
#include "scene/SceneSetup.h"
#include <iostream>
#include <string>

namespace
{
	//--- Resources (paths from the project folder) ---------------------------
	const char* MAP_MODEL    = "resources/models/aram/aram.obj";
	const char* BULB_MODEL   = "resources/models/light/light.obj";
	const char* ZED_MODEL    = "resources/models/characters/zed.glb";
	const std::string SHADER_DIR = "resources/shaders/";

	//--- Placement ------------------------------------------------------------

	// The map OBJ is normalized in [-1, 1]: scale it so that 1 unit ~ 1 meter
	const float MAP_SCALE = 500.0f;

	// Characters on the path in the middle of the lane (floor y = 6.06)
	const glm::vec3 ZED_POSITION(-5.0f, 6.06f, 0.0f);
	const float ZED_ROTATION = 180.0f;					// degrees around Y: face the blue side (+Z)
	const float CHARACTER_SCALE = 1.5f;

	// K/DA dancers: two on each side of Zed, facing the blue side like him
	// (on the path: a bush starts at x = 0.5)
	// (one model each in SceneResources::dancers, same order)
	struct Dancer
	{
		const char* name;
		const char* file;
		glm::vec3 position;
	};
	const Dancer DANCERS[4] = {
		{ "Ahri",    "resources/models/characters/k_da_ahri.glb",    glm::vec3(-10.2f, 6.06f, 0.0f) },
		{ "Akali",   "resources/models/characters/k_da_akali.glb",   glm::vec3( -7.6f, 6.06f, 0.0f) },
		{ "Evelynn", "resources/models/characters/k_da_evelynn.glb", glm::vec3( -2.4f, 6.06f, 0.0f) },
		{ "Kaisa",   "resources/models/characters/k_da_kaisa.glb",   glm::vec3(  0.2f, 6.06f, 0.0f) }
	};
	const float DANCER_ROTATION = 0.0f;			// their models face the other way than Zed's

	// Dead champions in front of Zed: static OBJ models (the last pose of
	// their death animation, exported with Blender), homework OBJ loader
	// (one model each in SceneResources::dead, same order)
	struct Corpse
	{
		const char* name;
		const char* file;
		glm::vec3 position;
		float rotation;		// degrees around Y
	};
	const Corpse DEAD[5] = {
		{ "Qiyana",   "resources/models/dead/qiyana/qiyana.obj",     glm::vec3( -5.0f, 6.06f, 4.0f),  0.0f },
		{ "Malphite", "resources/models/dead/malphite/malphite.obj", glm::vec3(-11.5f, 6.06f, 6.0f),  0.0f },
		{ "Irelia",   "resources/models/dead/irelia/irelia.obj",     glm::vec3( -8.5f, 6.06f, 5.0f),  0.0f },
		{ "Yunara",   "resources/models/dead/yunara/yunara.obj",     glm::vec3( -1.5f, 6.06f, 5.0f),  0.0f },
		{ "ChoGath",  "resources/models/dead/chogath/chogath.obj",   glm::vec3( -5.0f, 6.06f, 10.5f), 0.0f }	// big: in front of Qiyana
	};

	// Trump dancing at the other (red) end of the lane, facing the lane (+Z).
	// The tour ends on him.
	const char* TRUMP_MODEL = "resources/models/characters/trump_silly_dancing.glb";
	const float TRUMP_SCALE = 1.5f;

	// Lamps (point lights): blue nexus, red nexus, middle of the lane
	struct Lamp
	{
		const char* name;
		glm::vec3 position;
		glm::vec3 color;
	};
	const Lamp LAMPS[] = {
		{ "LampBlue",   glm::vec3(-5.0f, 13.0f,  92.0f), glm::vec3(0.2f, 0.45f, 1.0f) },
		{ "LampRed",    glm::vec3(-5.0f, 13.0f, -92.0f), glm::vec3(1.0f, 0.25f, 0.15f) },
		{ "LampMiddle", glm::vec3(-5.0f, 16.0f,   0.0f), glm::vec3(1.0f, 0.75f, 0.4f) }
	};
	const float BULB_SCALE = 3.0f;
	const float BULB_GLOW = 4.0f;		// color x 4: brighter than white, so the bloom makes it glow

	bool loadShader(ShaderProgram& shader, const std::string& vertexFile, const std::string& fragmentFile)
	{
		std::string vs = SHADER_DIR + vertexFile;
		std::string fs = SHADER_DIR + fragmentFile;
		return shader.loadShaders(vs.c_str(), fs.c_str());
	}

	SceneObject makeCharacter(const char* name, SkinnedModel& model, SceneResources& resources,
	                          const glm::vec3& position, float rotationDegrees, bool animated)
	{
		SceneObject object;
		object.name = name;
		object.skinned = &model;
		object.animated = animated;
		object.transform.position = position;
		object.transform.rotation.y = glm::radians(rotationDegrees);
		object.transform.scale = glm::vec3(CHARACTER_SCALE);
		object.material.shader = &resources.skinnedShader;
		object.material.receivesLighting = true;
		object.shadow = ShadowGroup::Dynamic;
		object.shadowShader = &resources.skinnedDepthShader;
		object.castsShadows = true;
		object.pointShadowShader = &resources.skinnedPointDepthShader;
		return object;
	}
}

//-----------------------------------------------------------------------------
// Loads the shaders and models, then adds the objects and the lights
//-----------------------------------------------------------------------------
bool SceneSetup::build(Scene& scene, LightManager& lights, SceneResources& resources)
{
	//--- Shaders --------------------------------------------------------------
	loadShader(resources.lightingShader, "lighting_dir_point_spot.vert", "scene.frag");
	loadShader(resources.skinnedShader, "skinned.vert", "scene.frag");
	loadShader(resources.bulbShader, "bulb.vert", "bulb.frag");
	loadShader(resources.depthShader, "shadow_depth.vert", "shadow_depth.frag");
	loadShader(resources.skinnedDepthShader, "skinned_shadow_depth.vert", "shadow_depth.frag");
	loadShader(resources.pointDepthShader, "point_shadow_depth.vert", "point_shadow_depth.frag");
	loadShader(resources.skinnedPointDepthShader, "skinned_point_shadow_depth.vert", "point_shadow_depth.frag");

	//--- Models ---------------------------------------------------------------
	if (!resources.map.loadOBJ(MAP_MODEL))
	{
		std::cerr << "Failed to load the map" << std::endl;
		return false;
	}
	resources.bulb.loadOBJ(BULB_MODEL);
	resources.zed.load(ZED_MODEL);

	//--- Objects --------------------------------------------------------------
	SceneObject map;
	map.name = "Map";
	map.mesh = &resources.map;
	map.transform.scale = glm::vec3(MAP_SCALE);
	map.material.shader = &resources.lightingShader;
	map.material.receivesLighting = true;
	map.material.setInt("winterMap", 1);
	map.shadow = ShadowGroup::Static;
	map.shadowShader = &resources.depthShader;
	map.castsShadows = true;
	map.pointShadowShader = &resources.pointDepthShader;
	scene.addObject(map);

	scene.addObject(makeCharacter("Zed", resources.zed, resources, ZED_POSITION, ZED_ROTATION, true));

	for (int i = 0; i < 4; i++)
	{
		resources.dancers[i].load(DANCERS[i].file);
		scene.addObject(makeCharacter(DANCERS[i].name, resources.dancers[i], resources,
		                              DANCERS[i].position, DANCER_ROTATION, true));
	}

	// Dead bodies: OBJ meshes drawn like the map, without its snow covering.
	// Shadows in the (sharper) character shadow map, with the OBJ depth shader.
	for (int i = 0; i < 5; i++)
	{
		resources.dead[i].loadOBJ(DEAD[i].file);

		SceneObject body;
		body.name = DEAD[i].name;
		body.mesh = &resources.dead[i];
		body.transform.position = DEAD[i].position;
		body.transform.rotation.y = glm::radians(DEAD[i].rotation);
		body.transform.scale = glm::vec3(CHARACTER_SCALE);
		body.material.shader = &resources.lightingShader;
		body.material.receivesLighting = true;
		body.material.setInt("winterMap", 0);
		body.shadow = ShadowGroup::Dynamic;
		body.shadowShader = &resources.depthShader;
		body.castsShadows = true;
		body.pointShadowShader = &resources.pointDepthShader;
		scene.addObject(body);
	}

	resources.trump.load(TRUMP_MODEL);
	SceneObject trump = makeCharacter("Trump", resources.trump, resources, TRUMP_POSITION, TRUMP_ROTATION, true);
	trump.transform.scale = glm::vec3(TRUMP_SCALE);
	trump.shadow = ShadowGroup::None;	// far from the character shadow map (middle of the lane)
	scene.addObject(trump);

	//--- Lamps: a point light + a glowing bulb each -----------------------------
	for (const Lamp& lamp : LAMPS)
	{
		PointLight light;
		light.position = lamp.position;
		light.color = lamp.color;
		light.castsShadows = true;		// cube shadow map (the bulb itself casts none)
		lights.addPointLight(light);

		SceneObject bulb;
		bulb.name = lamp.name;
		bulb.mesh = &resources.bulb;
		bulb.transform.position = lamp.position;
		bulb.transform.scale = glm::vec3(BULB_SCALE);
		bulb.material.shader = &resources.bulbShader;
		bulb.material.setVec3("lightColor", lamp.color * BULB_GLOW);
		scene.addObject(bulb);
	}

	setTimeOfDay(lights, true);
	return true;
}

//-----------------------------------------------------------------------------
// Night: cold moonlight, lamps brighter and reaching further.  Day: sun.
//-----------------------------------------------------------------------------
void SceneSetup::setTimeOfDay(LightManager& lights, bool night)
{
	DirectionalLight light;
	light.direction = LIGHT_DIRECTION;
	if (night)
	{
		light.ambient = glm::vec3(0.05f, 0.06f, 0.1f);
		light.diffuse = glm::vec3(0.45f, 0.5f, 0.7f);
		light.specular = glm::vec3(0.3f, 0.35f, 0.5f);
	}
	else
	{
		light.ambient = glm::vec3(0.35f, 0.35f, 0.4f);
		light.diffuse = glm::vec3(0.72f, 0.80f, 0.90f);
		light.specular = glm::vec3(0.5f, 0.5f, 0.5f);
	}
	lights.setDirectionalLight(light);

	for (PointLight& lamp : lights.getPointLights())
	{
		lamp.intensity = night ? 1.5f : 1.0f;
		lamp.linear = night ? 0.022f : 0.045f;
		lamp.quadratic = night ? 0.0019f : 0.0075f;
	}
}

//-----------------------------------------------------------------------------
// Color of the moon (night) or sun (day) disk drawn in the sky
//-----------------------------------------------------------------------------
glm::vec3 SceneSetup::skyLightColor(bool night)
{
	// Brighter than white: the bloom makes the disk glow
	return night ? glm::vec3(1.6f, 1.7f, 2.0f) : glm::vec3(6.0f, 5.4f, 4.4f);
}

//-----------------------------------------------------------------------------
// Background color
//-----------------------------------------------------------------------------
glm::vec4 SceneSetup::skyColor(bool night)
{
	return night ? glm::vec4(0.02f, 0.03f, 0.08f, 1.0f) : glm::vec4(0.55f, 0.68f, 0.82f, 1.0f);
}

