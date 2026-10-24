//-----------------------------------------------------------------------------
// Position, rotation and scale of an object -> its model matrix
//-----------------------------------------------------------------------------
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct Transform
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);	// Euler angles in radians
	glm::vec3 scale = glm::vec3(1.0f);

	// translate * rotate X * rotate Y * rotate Z * scale
	glm::mat4 getMatrix() const
	{
		glm::mat4 matrix(1.0f);
		matrix = glm::translate(matrix, position);
		matrix = glm::rotate(matrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		matrix = glm::rotate(matrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		matrix = glm::rotate(matrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		matrix = glm::scale(matrix, scale);
		return matrix;
	}
};
#endif //TRANSFORM_H
