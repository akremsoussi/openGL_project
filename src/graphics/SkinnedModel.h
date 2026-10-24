//-----------------------------------------------------------------------------
// Animated character loaded from a glTF binary file (.glb) with cgltf.
//
//  - Mesh:      vertices + the 4 bones that move each vertex (and their weights)
//  - Skeleton:  tree of nodes (bones), each with a position/rotation/scale
//  - Animation: keyframes of these positions/rotations/scales over time
//
// Each frame the skeleton pose is computed on the CPU (one matrix per bone)
// and the vertex shader moves every vertex with its bones ("skinning").
//-----------------------------------------------------------------------------
#ifndef SKINNEDMODEL_H
#define SKINNEDMODEL_H

#include <vector>
#include <string>
#define GLEW_STATIC
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

class ShaderProgram;
class Texture2D;

// Must match MAX_BONES in the skinned vertex shaders.  The bones are sent in a
// uniform buffer: 256 matrices = 16 KB, the size OpenGL 3.3 always allows.
const int MAX_BONES = 256;
const GLuint BONES_BINDING = 0;		// uniform buffer binding point of the "Bones" block

class SkinnedModel
{
public:
	 SkinnedModel();
	~SkinnedModel();

	// Loads the mesh, textures, skeleton and the first animation of the file
	bool load(const std::string& filename);

	void update(double elapsedTime);	// play the animation (loops)
	void setTime(float seconds);		// show the pose at this time
	float getDuration() const { return mDuration; }

	// The shader must be active and use the skinned vertex shader.
	// Sends the bone matrices (uniform block "Bones") and the material of
	// each part, then draws.
	void draw(ShaderProgram& shader);

private:
	SkinnedModel(const SkinnedModel& rhs);
	SkinnedModel& operator = (const SkinnedModel& rhs);

	struct SkinnedVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
		GLuint joints[4];		// bones moving this vertex
		glm::vec4 weights;		// how much each of them moves it (sum = 1)
	};

	struct Part					// triangles drawn with one material
	{
		GLuint start;			// first index
		GLuint count;			// number of indices
		int material;
	};

	struct Material
	{
		glm::vec3 color;
		Texture2D* texture;		// NULL if none
	};

	struct Node					// node of the skeleton tree
	{
		int parent;				// -1 for a root
		glm::vec3 translation;	// rest pose (changed by the animation)
		glm::quat rotation;
		glm::vec3 scale;
		bool hasMatrix;			// some files give a matrix instead of T/R/S
		glm::mat4 matrix;
		glm::mat4 global;		// node -> model space, for the current pose
	};

	enum ChannelPath { TRANSLATION, ROTATION, SCALE };

	struct Channel				// keyframes of one property of one node
	{
		int node;
		ChannelPath path;
		bool step;				// no interpolation between keys
		std::vector<float> times;
		std::vector<glm::vec4> values;
	};

	void computeNormals(std::vector<SkinnedVertex>& vertices, const std::vector<GLuint>& indices);
	void initBuffers(const std::vector<SkinnedVertex>& vertices, const std::vector<GLuint>& indices);
	glm::vec4 sample(const Channel& channel, float time) const;
	void computePose();

	GLuint mVAO, mVBO, mEBO;
	GLuint mBoneBuffer;					// uniform buffer holding mBones for the shader
	std::vector<Part> mParts;
	std::vector<Material> mMaterials;
	std::vector<Texture2D*> mTextures;

	std::vector<Node> mRestPose;		// skeleton without animation
	std::vector<Node> mNodes;			// skeleton in the current pose
	std::vector<int> mNodeOrder;		// parents always before their children
	std::vector<int> mJointNodes;		// node of each bone
	std::vector<glm::mat4> mInverseBind;	// model space -> bone space (rest pose)
	std::vector<glm::mat4> mBones;		// final matrix of each bone, sent to the shader

	std::vector<Channel> mChannels;
	float mDuration;
	float mTime;
};
#endif //SKINNEDMODEL_H
