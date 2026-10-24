//-----------------------------------------------------------------------------
// The objects to draw, found by name.  The models and shaders they use are
// owned by SceneResources.
//-----------------------------------------------------------------------------
#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>
#include "scene/SceneObject.h"

struct Scene
{
	std::vector<SceneObject> objects;

	void addObject(const SceneObject& object) { objects.push_back(object); }

	SceneObject* findObject(const std::string& name)
	{
		for (SceneObject& object : objects)
		{
			if (object.name == name)
				return &object;
		}
		return nullptr;
	}
};
#endif //SCENE_H
