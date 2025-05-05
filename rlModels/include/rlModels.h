
#pragma once

#include "raylib.h"
#include "raymath.h"

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

	typedef struct rlmPQSTransorm // a transform with a position, quaternion rotation, scale
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
	}rlmPQSTransorm;

	typedef struct rlmGPUMesh	// minimum data needed to draw a mesh on the GPU
	{
		unsigned int vaoId;
		unsigned int* vboIds;

		bool isIndexed;
		unsigned int elementCount;
	}rlmGPUMesh;

	typedef struct rlmMeshBuffers	//CPU geometry buffers used to define a mesh
	{
		int vertexCount;        // Number of vertices stored in arrays
		int triangleCount;      // Number of triangles stored (indexed or not)

		float* vertices;        // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
		float* texcoords;       // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
		float* texcoords2;      // Vertex texture second coordinates (UV - 2 components per vertex) (shader-location = 5)
		float* normals;         // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
		float* tangents;        // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
		unsigned char* colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
		unsigned short* indices;    // Vertex indices (in case vertex data comes indexed)

		unsigned char* boneIds; // Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning) (shader-location = 6)
		float* boneWeights;     // Vertex bone weight, up to 4 bones influence by vertex (skinning) (shader-location = 7)
	}rlmMeshBuffers;

	typedef struct rlmMesh // a mesh
	{
		char* name;
		rlmGPUMesh gpuMesh;
		BoundingBox bounds;

		rlmMeshBuffers* meshBuffers; // optional after upload

		rlmPQSTransorm transform;

		// TODO rlmAnimatedMeshBuffers ?
	}rlmMesh;

	typedef struct rlmMaterialChannel // a texture map in a material
	{
		int textureId;
		int textureLoc;
		int textureSlot;

		Color color;
		int colorLoc;

		bool cubeMap;
		bool ownsTexture;
	}rlmMaterialChannel;

	typedef struct rlmMaterialValueF
	{
		int shaderLoc;
		float value;
	}rlmMaterialValueF;

	typedef struct rlmMaterialDef // a shader and it's input texture
	{
		char* name;
		Shader shader;
		bool ownsShader;

		rlmMaterialChannel	baseChannel;

		int materialChannels;
		rlmMaterialChannel* extraChannels;

		int materialValues;
		rlmMaterialValueF* values;
	}rlmMaterialDef;

	typedef struct rlmModelGroup // a group of meshes that share a material
	{
		rlmMaterialDef material;
		bool ownsMeshes;
		bool ownsMeshList;
		int meshCount;
		rlmMesh* meshes;
		bool* meshDisableFlags;
	}rlmModelGroup;

	typedef struct rlmBoneInfo  // a node in the bone tree
	{
		char name[32];

		int boneId;
		int parentId;

		int childCount;
		struct rlmBoneInfo** childBones;
	}rlmBoneInfo;

	typedef struct rlmAnimationKeyframe // a list of bone transforms for a skeleton
	{
		rlmPQSTransorm* boneTransforms;
	}rlmAnimationKeyframe;

	typedef struct rlmSkeleton // a tree of bones, and the keyframe for it's binding pose
	{
		int boneCount;
		rlmBoneInfo* bones;

		rlmBoneInfo* rootBone;

		rlmAnimationKeyframe bindingFrame;
	}rlmSkeleton;

	typedef struct rlmModelAnimationPose    // a list of model space matricides baked out for display of an animation
	{
		Matrix* boneMatricies;
	}rlmModelAnimationPose;

	typedef struct rlmModelAniamtionSequence    // a named sequence of keyframes
	{
		char name[64];
		float fps;

		int keyframeCount;
		rlmAnimationKeyframe* keyframes;
	}rlmModelAniamtionSequence;

	typedef struct rlmModelAnimationSet
	{
		int sequenceCount;
		rlmModelAniamtionSequence* sequences;
	}rlmModelAnimationSet;

	typedef struct rlmModel // a group of meshes, materials, and an orientation transform
	{
		int groupCount;
		rlmModelGroup* groups;
		rlmPQSTransorm orientationTransform;

		bool ownsSkeleton;
		rlmSkeleton* skeleton;
	}rlmModel;

	typedef struct rlmAnimatedModelInstance
	{
		rlmModel* model;
		rlmModelAnimationSet* sequences;

		rlmModelAnimationPose currentPose;

		int currentSequence;
		int currentFrame;

		bool interpolate;

		float currentParam;

		rlmPQSTransorm transform;
	}rlmAnimatedModelInstance;

	// meshes
	void rlmUploadMesh(rlmMesh* mesh, bool releaseGeoBuffers);

	// materials
	rlmMaterialDef rlmGetDefaultMaterial();

	void rlmUnloadMaterial(rlmMaterialDef* material);

	void rlmAddMaterialChannel(rlmMaterialDef* material, Texture2D texture, int shaderLoc, int slot, bool isCubeMap);
	void rlmAddMaterialChannels(rlmMaterialDef* material, int count, Texture2D* textures, int* locs);

	int rlmAddMaterialValue(rlmMaterialDef* material, int shaderLoc, float value);
	void rlmSetMaterialValue(rlmMaterialDef* material, int valueIndex, float value);

	void rlmSetMaterialDefShader(rlmMaterialDef* material, Shader shader);

	void rlmSetMaterialChannelTexture(rlmMaterialChannel* channel, Texture2D texture);

	// models
	void rlmSetModelShader(rlmModel* model, Shader shader);

	rlmModel rlmCloneModel(rlmModel model);

	void rlmUnloadModel(rlmModel* model);

	void rlmApplyMaterialDef(rlmMaterialDef* material);

	void rlmDrawModel(rlmModel model, rlmPQSTransorm transform);

	void rlmDrawModelWithPose(rlmModel model, rlmPQSTransorm transform, rlmModelAnimationPose* pose);

	void rlmDrawModelWithPoseEx(rlmModel model, rlmPQSTransorm transform, rlmModelAnimationPose* pose, Shader* shader);


	// animations
	rlmModelAnimationPose rlmLoadPoseFromModel(rlmModel model);
	void rlmUnloadPose(rlmModelAnimationPose* pose);

	void rlmSetPoseToKeyframe(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame);
	void rlmSetPoseToKeyframeEx(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame, rlmBoneInfo* startBone);

	void rlmSetPoseToKeyframesLerp(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame1, rlmAnimationKeyframe frame2, float param);
	void rlmSetPoseToKeyframesLerpEx(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame1, rlmAnimationKeyframe frame2, float param, rlmBoneInfo* startBone);

	rlmBoneInfo* rlmFindBoneByName(rlmModel model, const char* boneName);

	void rlmAdvanceAnimationInstance(rlmAnimatedModelInstance* instance, float deltaTime);
	void rlmSetAnimationInstanceSequence(rlmAnimatedModelInstance* instance, int sequence);

	void rlmUnloadAnimationPose(rlmModelAnimationPose* pose);
	void rlmUnloadAnimationKeyframe(rlmAnimationKeyframe* keyframe);
	void rlmUnloadAnimationSequence(rlmModelAniamtionSequence* sequence);
	void rlmUnloadAnimationSet(rlmModelAnimationSet* set);

	// transform utility
	rlmPQSTransorm rlmPQSIdentity();
	rlmPQSTransorm rlmPQSTranslation(float x, float y, float z);

	rlmPQSTransorm rlmPQSTransformAdd(rlmPQSTransorm lhs, rlmPQSTransorm rhs);
	rlmPQSTransorm rlmPQSTransformSubtract(rlmPQSTransorm lhs, rlmPQSTransorm rhs);

	rlmPQSTransorm rlmPQSLerp(const rlmPQSTransorm* lhs, const rlmPQSTransorm* rhs, float param);

	Matrix rlmPQSToMatrix(const rlmPQSTransorm* transform);
	rlmPQSTransorm rlmPQSFromMatrix(Matrix matrix);

	// state API
	void rlmSetDefaultMaterialShader(Shader shader);
	void rlmClearDefaultMaterialShader();

#if defined (RLMODELS_IMPLEMENTATION)
	// TODO put the guts here once it all works
#endif

#if defined(__cplusplus)
}
#endif