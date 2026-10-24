//-----------------------------------------------------------------------------
// One object of the scene: a name, a transform, a model (OBJ mesh or animated
// glTF character, owned by SceneResources) and a material.
//-----------------------------------------------------------------------------
#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <string>
#include "Mesh.h"
#include "graphics/SkinnedModel.h"
#include "scene/Material.h"
#include "scene/Transform.h"

// Which shadow map the object is drawn into
enum class ShadowGroup
{
	None,		// casts no shadow (ex: lamps)
	Static,		// never moves: drawn once into the map shadow map
	Dynamic		// characters (animated or not): drawn every frame into the
				// smaller, sharper character shadow map
};

struct SceneObject
{
	std::string name;
	bool visible = true;
	Transform transform;

	Mesh* mesh = nullptr;				// one of the two
	SkinnedModel* skinned = nullptr;
	bool animated = false;				// skinned: play its animation every frame

	Material material;

	// Shadows of the moon: which of its two shadow maps
	ShadowGroup shadow = ShadowGroup::None;
	ShaderProgram* shadowShader = nullptr;		// depth shader (moon and flashlight maps)

	// Shadows of the lamps and of the flashlight
	bool castsShadows = false;
	ShaderProgram* pointShadowShader = nullptr;	// distance shader for the lamp cube maps

	// Draws the model with the current (active) shader
	void draw(ShaderProgram& shader)
	{
		if (mesh != nullptr)
			mesh->draw(shader);
		else if (skinned != nullptr)
			skinned->draw(shader);
	}
};
#endif //SCENEOBJECT_H
