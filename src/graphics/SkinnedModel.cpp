//-----------------------------------------------------------------------------
// Animated character loaded from a glTF binary file (.glb) with cgltf.
// See SkinnedModel.h
//-----------------------------------------------------------------------------
#include "graphics/SkinnedModel.h"
#include "ShaderProgram.h"
#include "Texture2D.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <tuple>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
SkinnedModel::SkinnedModel()
	: mVAO(0), mVBO(0), mEBO(0), mBoneBuffer(0), mDuration(0.0f), mTime(0.0f)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
SkinnedModel::~SkinnedModel()
{
	glDeleteVertexArrays(1, &mVAO);
	glDeleteBuffers(1, &mVBO);
	glDeleteBuffers(1, &mEBO);
	glDeleteBuffers(1, &mBoneBuffer);

	for (size_t i = 0; i < mTextures.size(); i++)
		delete mTextures[i];
}

//-----------------------------------------------------------------------------
// Loads the .glb: skinned mesh, textures, skeleton and first animation
//-----------------------------------------------------------------------------
bool SkinnedModel::load(const std::string& filename)
{
	std::cout << "Loading glTF file " << filename << " ..." << std::endl;

	cgltf_options options = {};
	cgltf_data* data = NULL;
	if (cgltf_parse_file(&options, filename.c_str(), &data) != cgltf_result_success ||
		cgltf_load_buffers(&options, data, filename.c_str()) != cgltf_result_success)
	{
		std::cerr << "Cannot load " << filename << std::endl;
		cgltf_free(data);
		return false;
	}

	// The first node that has both a mesh and a skin is the character (some
	// files split it into several mesh nodes sharing that skin: see below)
	cgltf_node* meshNode = NULL;
	for (cgltf_size i = 0; i < data->nodes_count && meshNode == NULL; i++)
	{
		if (data->nodes[i].mesh != NULL && data->nodes[i].skin != NULL)
			meshNode = &data->nodes[i];
	}

	if (meshNode == NULL || meshNode->skin->joints_count > (cgltf_size)MAX_BONES)
	{
		std::cerr << "No skinned mesh (or more than " << MAX_BONES << " bones) in " << filename << std::endl;
		cgltf_free(data);
		return false;
	}

	//--- Skeleton: every node with its rest position/rotation/scale ---------
	mRestPose.resize(data->nodes_count);
	for (cgltf_size i = 0; i < data->nodes_count; i++)
	{
		const cgltf_node& n = data->nodes[i];
		Node& node = mRestPose[i];

		node.parent = n.parent ? (int)cgltf_node_index(data, n.parent) : -1;
		node.translation = n.has_translation ? glm::make_vec3(n.translation) : glm::vec3(0.0f);
		node.rotation = n.has_rotation ? glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);	// glTF: x y z w
		node.scale = n.has_scale ? glm::make_vec3(n.scale) : glm::vec3(1.0f);
		node.hasMatrix = (n.has_matrix != 0);
		node.matrix = n.has_matrix ? glm::make_mat4(n.matrix) : glm::mat4(1.0f);
		node.global = glm::mat4(1.0f);
	}

	// Order the nodes so that a parent is always computed before its children
	std::vector<int> stack;
	for (int i = (int)data->nodes_count - 1; i >= 0; i--)
	{
		if (mRestPose[i].parent < 0)
			stack.push_back(i);
	}
	while (!stack.empty())
	{
		int node = stack.back();
		stack.pop_back();
		mNodeOrder.push_back(node);

		const cgltf_node& n = data->nodes[node];
		for (cgltf_size c = 0; c < n.children_count; c++)
			stack.push_back((int)cgltf_node_index(data, n.children[c]));
	}

	// Bones of the skin and the matrix bringing the mesh into each bone space
	const cgltf_skin* skin = meshNode->skin;
	for (cgltf_size j = 0; j < skin->joints_count; j++)
	{
		mJointNodes.push_back((int)cgltf_node_index(data, skin->joints[j]));

		glm::mat4 inverseBind(1.0f);
		if (skin->inverse_bind_matrices)
			cgltf_accessor_read_float(skin->inverse_bind_matrices, j, glm::value_ptr(inverseBind), 16);
		mInverseBind.push_back(inverseBind);
	}

	//--- Textures (images stored inside the .glb, or next to it) ------------
	std::string directory = filename.substr(0, filename.find_last_of("/\\") + 1);
	mTextures.resize(data->images_count, NULL);
	for (cgltf_size i = 0; i < data->images_count; i++)
	{
		const cgltf_image& image = data->images[i];
		Texture2D* texture = new Texture2D();
		bool ok = false;

		if (image.buffer_view != NULL)
		{
			const unsigned char* bytes = (const unsigned char*)image.buffer_view->buffer->data + image.buffer_view->offset;
			ok = texture->loadTextureFromMemory(bytes, (int)image.buffer_view->size, true);
		}
		else if (image.uri != NULL)
		{
			ok = texture->loadTexture(directory + image.uri, true);
		}

		if (ok)
			mTextures[i] = texture;
		else
			delete texture;
	}

	//--- Materials: base color and texture ----------------------------------
	for (cgltf_size i = 0; i < data->materials_count; i++)
	{
		const cgltf_material& m = data->materials[i];
		Material material;
		material.color = glm::vec3(1.0f);
		material.texture = NULL;

		// Base color: metallic-roughness materials (most files), or the older
		// specular-glossiness extension (ex: Sketchfab exports) where it is "diffuse"
		const cgltf_texture_view* colorTexture = NULL;
		if (m.has_pbr_metallic_roughness)
		{
			material.color = glm::make_vec3(m.pbr_metallic_roughness.base_color_factor);
			colorTexture = &m.pbr_metallic_roughness.base_color_texture;
		}
		if (m.has_pbr_specular_glossiness)
		{
			material.color = glm::make_vec3(m.pbr_specular_glossiness.diffuse_factor);
			colorTexture = &m.pbr_specular_glossiness.diffuse_texture;
		}
		if (colorTexture && colorTexture->texture && colorTexture->texture->image)
			material.texture = mTextures[cgltf_image_index(data, colorTexture->texture->image)];

		mMaterials.push_back(material);
	}

	//--- Mesh: one part per primitive, all in the same buffers --------------
	std::vector<SkinnedVertex> vertices;
	std::vector<GLuint> indices;
	bool hasNormals = true;

	// Primitives of every mesh node moved by this skeleton
	std::vector<const cgltf_primitive*> primitives;
	for (cgltf_size i = 0; i < data->nodes_count; i++)
	{
		const cgltf_node& n = data->nodes[i];
		if (n.mesh == NULL || n.skin != skin)
			continue;
		for (cgltf_size p = 0; p < n.mesh->primitives_count; p++)
			primitives.push_back(&n.mesh->primitives[p]);
	}

	for (size_t p = 0; p < primitives.size(); p++)
	{
		const cgltf_primitive& primitive = *primitives[p];
		if (primitive.type != cgltf_primitive_type_triangles)
			continue;

		const cgltf_accessor *positions = NULL, *normals = NULL, *uvs = NULL, *joints = NULL, *weights = NULL;
		for (cgltf_size a = 0; a < primitive.attributes_count; a++)
		{
			const cgltf_attribute& attribute = primitive.attributes[a];
			if (attribute.index != 0)
				continue;
			switch (attribute.type)
			{
			case cgltf_attribute_type_position: positions = attribute.data; break;
			case cgltf_attribute_type_normal:   normals = attribute.data; break;
			case cgltf_attribute_type_texcoord: uvs = attribute.data; break;
			case cgltf_attribute_type_joints:   joints = attribute.data; break;
			case cgltf_attribute_type_weights:  weights = attribute.data; break;
			default: break;
			}
		}
		if (positions == NULL)
			continue;
		hasNormals = hasNormals && (normals != NULL);

		GLuint firstVertex = (GLuint)vertices.size();
		for (cgltf_size v = 0; v < positions->count; v++)
		{
			SkinnedVertex vertex;
			vertex.position = glm::vec3(0.0f);
			vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			vertex.texCoords = glm::vec2(0.0f);
			vertex.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			GLuint jointIndices[4] = { 0, 0, 0, 0 };

			cgltf_accessor_read_float(positions, v, glm::value_ptr(vertex.position), 3);
			if (normals)
				cgltf_accessor_read_float(normals, v, glm::value_ptr(vertex.normal), 3);
			if (uvs)
			{
				cgltf_accessor_read_float(uvs, v, glm::value_ptr(vertex.texCoords), 2);
				vertex.texCoords.y = 1.0f - vertex.texCoords.y;	// glTF: v goes down, OpenGL: up
			}
			if (joints)
				cgltf_accessor_read_uint(joints, v, jointIndices, 4);
			if (weights)
			{
				cgltf_accessor_read_float(weights, v, glm::value_ptr(vertex.weights), 4);
				float sum = vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w;
				vertex.weights = (sum > 0.0f) ? vertex.weights / sum : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			}
			for (int k = 0; k < 4; k++)
				vertex.joints[k] = jointIndices[k];

			vertices.push_back(vertex);
		}

		Part part;
		part.start = (GLuint)indices.size();
		part.material = primitive.material ? (int)cgltf_material_index(data, primitive.material) : -1;

		if (primitive.indices)
		{
			for (cgltf_size i = 0; i < primitive.indices->count; i++)
				indices.push_back(firstVertex + (GLuint)cgltf_accessor_read_index(primitive.indices, i));
		}
		else
		{
			for (cgltf_size i = 0; i < positions->count; i++)
				indices.push_back(firstVertex + (GLuint)i);
		}

		part.count = (GLuint)indices.size() - part.start;
		mParts.push_back(part);
	}

	//--- Animation: keyframes of the first animation ------------------------
	if (data->animations_count > 0)
	{
		const cgltf_animation& animation = data->animations[0];
		for (cgltf_size c = 0; c < animation.channels_count; c++)
		{
			const cgltf_animation_channel& ch = animation.channels[c];
			if (ch.target_node == NULL || ch.target_path == cgltf_animation_path_type_weights)
				continue;

			Channel channel;
			channel.node = (int)cgltf_node_index(data, ch.target_node);
			channel.path = (ch.target_path == cgltf_animation_path_type_translation) ? TRANSLATION :
			               (ch.target_path == cgltf_animation_path_type_rotation) ? ROTATION : SCALE;
			channel.step = (ch.sampler->interpolation == cgltf_interpolation_type_step);

			// Cubic spline keys are stored as (in tangent, value, out tangent):
			// keep the value only
			bool cubic = (ch.sampler->interpolation == cgltf_interpolation_type_cubic_spline);
			int components = (channel.path == ROTATION) ? 4 : 3;

			for (cgltf_size k = 0; k < ch.sampler->input->count; k++)
			{
				float time = 0.0f;
				cgltf_accessor_read_float(ch.sampler->input, k, &time, 1);

				glm::vec4 value(0.0f);
				cgltf_accessor_read_float(ch.sampler->output, cubic ? 3 * k + 1 : k, glm::value_ptr(value), components);

				channel.times.push_back(time);
				channel.values.push_back(value);
				mDuration = std::max(mDuration, time);
			}
			mChannels.push_back(channel);
		}
		std::cout << "  animation '" << (animation.name ? animation.name : "") << "' " << mDuration << " s" << std::endl;
	}

	std::cout << "  " << vertices.size() << " vertices, " << indices.size() / 3 << " triangles, "
		<< mJointNodes.size() << " bones, " << mMaterials.size() << " materials" << std::endl;

	cgltf_free(data);

	if (!hasNormals)
		computeNormals(vertices, indices);

	initBuffers(vertices, indices);

	mBones.resize(mJointNodes.size(), glm::mat4(1.0f));
	setTime(0.0f);
	return true;
}

//-----------------------------------------------------------------------------
// The file has no normals: average the normals of the triangles around each
// vertex.  Vertices at the same place (UV seams) share their normal so the
// seams do not show.
//-----------------------------------------------------------------------------
void SkinnedModel::computeNormals(std::vector<SkinnedVertex>& vertices, const std::vector<GLuint>& indices)
{
	std::map<std::tuple<float, float, float>, glm::vec3> normalAt;

	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const glm::vec3& a = vertices[indices[i]].position;
		const glm::vec3& b = vertices[indices[i + 1]].position;
		const glm::vec3& c = vertices[indices[i + 2]].position;
		glm::vec3 faceNormal = glm::cross(b - a, c - a);	// length = 2 x area: big triangles count more

		for (int k = 0; k < 3; k++)
		{
			const glm::vec3& p = vertices[indices[i + k]].position;
			normalAt[std::make_tuple(p.x, p.y, p.z)] += faceNormal;
		}
	}

	for (size_t i = 0; i < vertices.size(); i++)
	{
		const glm::vec3& p = vertices[i].position;
		glm::vec3 n = normalAt[std::make_tuple(p.x, p.y, p.z)];
		vertices[i].normal = (glm::length(n) > 0.0f) ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
	}
}

//-----------------------------------------------------------------------------
// Vertex buffer, index buffer and vertex array object
//-----------------------------------------------------------------------------
void SkinnedModel::initBuffers(const std::vector<SkinnedVertex>& vertices, const std::vector<GLuint>& indices)
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mEBO);

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SkinnedVertex), &vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

	GLsizei stride = sizeof(SkinnedVertex);

	// Same locations as the Mesh class for position, normal and texture coords
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(SkinnedVertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(SkinnedVertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(SkinnedVertex, texCoords));
	glEnableVertexAttribArray(2);

	// Bone indices are integers: glVertexAttribIPointer (no conversion to float)
	glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, stride, (GLvoid*)offsetof(SkinnedVertex, joints));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(SkinnedVertex, weights));
	glEnableVertexAttribArray(4);

	glBindVertexArray(0);

	// Bone matrices: filled at each draw
	glGenBuffers(1, &mBoneBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, mBoneBuffer);
	glBufferData(GL_UNIFORM_BUFFER, MAX_BONES * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//-----------------------------------------------------------------------------
// Plays the animation (loops at the end)
//-----------------------------------------------------------------------------
void SkinnedModel::update(double elapsedTime)
{
	if (mDuration <= 0.0f)
		return;

	setTime((float)fmod(mTime + elapsedTime, (double)mDuration));
}

//-----------------------------------------------------------------------------
// Computes the pose at this time (clamped to the animation length)
//-----------------------------------------------------------------------------
void SkinnedModel::setTime(float seconds)
{
	mTime = glm::clamp(seconds, 0.0f, mDuration);
	computePose();
}

//-----------------------------------------------------------------------------
// Value of a channel at a given time: interpolation between the two
// keyframes around it (linear, or spherical for rotations)
//-----------------------------------------------------------------------------
glm::vec4 SkinnedModel::sample(const Channel& channel, float time) const
{
	const std::vector<float>& times = channel.times;

	if (time <= times.front())
		return channel.values.front();
	if (time >= times.back())
		return channel.values.back();

	// First key after "time"
	size_t next = std::upper_bound(times.begin(), times.end(), time) - times.begin();
	size_t previous = next - 1;

	if (channel.step)
		return channel.values[previous];

	float t = (time - times[previous]) / (times[next] - times[previous]);
	const glm::vec4& a = channel.values[previous];
	const glm::vec4& b = channel.values[next];

	if (channel.path == ROTATION)
	{
		glm::quat q = glm::slerp(glm::quat(a.w, a.x, a.y, a.z), glm::quat(b.w, b.x, b.y, b.z), t);
		return glm::vec4(q.x, q.y, q.z, q.w);
	}
	return glm::mix(a, b, t);
}

//-----------------------------------------------------------------------------
// Skeleton pose at mTime -> one matrix per bone
//-----------------------------------------------------------------------------
void SkinnedModel::computePose()
{
	// 1. Start from the rest pose and apply the animation
	mNodes = mRestPose;
	for (size_t c = 0; c < mChannels.size(); c++)
	{
		const Channel& channel = mChannels[c];
		glm::vec4 value = sample(channel, mTime);
		Node& node = mNodes[channel.node];

		if (channel.path == TRANSLATION)
			node.translation = glm::vec3(value);
		else if (channel.path == ROTATION)
			node.rotation = glm::quat(value.w, value.x, value.y, value.z);
		else
			node.scale = glm::vec3(value);
	}

	// 2. Local matrices -> global matrices (parent x local), parents first
	for (size_t i = 0; i < mNodeOrder.size(); i++)
	{
		Node& node = mNodes[mNodeOrder[i]];

		glm::mat4 local = node.hasMatrix ? node.matrix :
			glm::translate(glm::mat4(1.0f), node.translation) * glm::mat4_cast(node.rotation) * glm::scale(glm::mat4(1.0f), node.scale);

		node.global = (node.parent >= 0) ? mNodes[node.parent].global * local : local;
	}

	// 3. Bone matrix: rest pose vertex -> bone space -> posed bone -> model space
	for (size_t j = 0; j < mJointNodes.size(); j++)
		mBones[j] = mNodes[mJointNodes[j]].global * mInverseBind[j];
}

//-----------------------------------------------------------------------------
// Draws the character in its current pose
//-----------------------------------------------------------------------------
void SkinnedModel::draw(ShaderProgram& shader)
{
	if (mVAO == 0)
		return;

	// Bone matrices -> uniform buffer, connected to the "Bones" block of the shader
	glBindBuffer(GL_UNIFORM_BUFFER, mBoneBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, mBones.size() * sizeof(glm::mat4), glm::value_ptr(mBones[0]));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, BONES_BINDING, mBoneBuffer);

	GLuint block = glGetUniformBlockIndex(shader.getProgram(), "Bones");
	if (block != GL_INVALID_INDEX)
		glUniformBlockBinding(shader.getProgram(), block, BONES_BINDING);

	glBindVertexArray(mVAO);
	for (size_t i = 0; i < mParts.size(); i++)
	{
		const Part& part = mParts[i];
		Material material = { glm::vec3(0.8f), NULL };
		if (part.material >= 0)
			material = mMaterials[part.material];

		shader.setUniform("material.ambient", glm::vec3(1.0f));
		shader.setUniform("material.diffuse", material.color);
		shader.setUniform("material.specular", glm::vec3(0.3f));
		shader.setUniform("material.shininess", 32.0f);
		shader.setUniform("material.hasDiffuseMap", (GLint)(material.texture != NULL));

		if (material.texture != NULL)
			material.texture->bind(0);

		glDrawElements(GL_TRIANGLES, part.count, GL_UNSIGNED_INT, (GLvoid*)(part.start * sizeof(GLuint)));
	}
	glBindVertexArray(0);
}
