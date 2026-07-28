#include "rlModels_IO.h"

#include <stdio.h>
#include <string.h>

rlmModel rlmLoadFromModel(Model raylibModel)
{
	return rlmLoadFromModelEX(raylibModel, false);
}

rlmModel rlmLoadFromModelEX(Model raylibModel, bool keepCPUdata)
{
	rlmModel newModel = { 0 };

	if (raylibModel.skeleton.boneCount > 0 && raylibModel.skeleton.bones != NULL)
	{
		newModel.ownsSkeleton = true;

		rlmSkeleton* skeleton = (rlmSkeleton*)MemAlloc(sizeof(rlmSkeleton));

		skeleton->boneCount = raylibModel.skeleton.boneCount;
		skeleton->bones = (rlmBoneInfo*)MemAlloc(sizeof(rlmBoneInfo) * skeleton->boneCount);

		skeleton->bindingFrame.boneTransforms = (rlmPQSTransorm*)MemAlloc(sizeof(rlmPQSTransorm) * skeleton->boneCount);

		for (int i = 0; i < skeleton->boneCount; i++)
		{
			skeleton->bones[i].boneId = i;
			skeleton->bones[i].name[0] = '\0';
			strcpy(skeleton->bones[i].name, raylibModel.skeleton.bones[i].name);

			skeleton->bones[i].parentId = raylibModel.skeleton.bones[i].parent;
			skeleton->bones[i].childBones = NULL;
			skeleton->bones[i].childCount = 0;

			skeleton->bindingFrame.boneTransforms[i].position = raylibModel.skeleton.bindPose[i].translation;
			skeleton->bindingFrame.boneTransforms[i].scale = raylibModel.skeleton.bindPose[i].scale;
			skeleton->bindingFrame.boneTransforms[i].rotation = raylibModel.skeleton.bindPose[i].rotation;
		}

		for (int i = 0; i < skeleton->boneCount; i++)
		{
			if (skeleton->bones[i].parentId < 0 && !skeleton->rootBone)
			{
				skeleton->rootBone = &skeleton->bones[i];
			}
			else if (skeleton->bones[i].parentId >= 0)
			{
				rlmBoneInfo* parentBone = &skeleton->bones[skeleton->bones[i].parentId];
				parentBone->childCount++;
				parentBone->childBones = (rlmBoneInfo**)MemRealloc(parentBone->childBones, sizeof(rlmBoneInfo*) * parentBone->childCount);
				parentBone->childBones[parentBone->childCount - 1] = &skeleton->bones[i];
			}
			else
			{
				TraceLog(LOG_WARNING, "rlModels : More than one bone has no parent, multiple roots, %s", skeleton->bones[i].name);
			}
		}

		// TODO, make children relative

		newModel.skeleton = skeleton;

		MemFree(raylibModel.skeleton.bindPose);
		MemFree(raylibModel.skeleton.bones);
	}

	newModel.orientationTransform = rlmPQSIdentity();

	newModel.groupCount = raylibModel.materialCount;
	newModel.groups = (rlmModelGroup*)MemAlloc(sizeof(rlmModelGroup) * newModel.groupCount);

	for (int groupIndex = 0; groupIndex < newModel.groupCount; groupIndex++)
	{
		rlmModelGroup* newGroup = newModel.groups + groupIndex;

		newGroup->ownsMeshes = true;
		newGroup->ownsMeshList = true;

		newGroup->material.name = (char*)MemAlloc(32);
		newGroup->material.name[0] = '\0';
		sprintf(newGroup->material.name, "imported_mat_%d", groupIndex);

		newGroup->material.shader = raylibModel.materials[groupIndex].shader;
		newGroup->material.ownsShader = true;

		newGroup->material.baseChannel.cubeMap = false;
		newGroup->material.baseChannel.textureId = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_DIFFUSE].texture.id;
		newGroup->material.baseChannel.textureSlot = 0;
		newGroup->material.baseChannel.ownsTexture = newGroup->material.baseChannel.textureId > 1;
		newGroup->material.baseChannel.textureLoc = -1;

		newGroup->material.baseChannel.color = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_DIFFUSE].color;
		newGroup->material.baseChannel.colorLoc = -SHADER_LOC_COLOR_DIFFUSE;

		newGroup->material.materialChannels = 0;

		if (raylibModel.materials[groupIndex].maps[MATERIAL_MAP_SPECULAR].texture.id > 0)
			newGroup->material.materialChannels++;
		if (raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id > 0)
			newGroup->material.materialChannels++;

		newGroup->material.extraChannels = (rlmMaterialChannel*)MemAlloc(sizeof(rlmMaterialChannel) * newGroup->material.materialChannels);

		int extraIndex = 0;
		if (raylibModel.materials[groupIndex].maps[MATERIAL_MAP_METALNESS].texture.id > 0)
		{
			newGroup->material.extraChannels[extraIndex].cubeMap = false;
			newGroup->material.extraChannels[extraIndex].textureId = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_METALNESS].texture.id;
			newGroup->material.extraChannels[extraIndex].textureSlot = 1;
			newGroup->material.extraChannels[extraIndex].ownsTexture = newGroup->material.baseChannel.textureId > 1;
			newGroup->material.extraChannels[extraIndex].textureLoc = newGroup->material.shader.locs[SHADER_LOC_MAP_METALNESS];

			newGroup->material.extraChannels[extraIndex].color = raylibModel.materials[groupIndex].maps[SHADER_LOC_MAP_METALNESS].color;
			newGroup->material.extraChannels[extraIndex].colorLoc = -SHADER_LOC_COLOR_SPECULAR;

			extraIndex++;
		}

		if (raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id > 0)
		{
			newGroup->material.extraChannels[extraIndex].cubeMap = false;
			newGroup->material.extraChannels[extraIndex].textureId = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id;
			newGroup->material.extraChannels[extraIndex].textureSlot = 2;
			newGroup->material.extraChannels[extraIndex].ownsTexture = newGroup->material.baseChannel.textureId > 1;
			newGroup->material.extraChannels[extraIndex].textureLoc = newGroup->material.shader.locs[SHADER_LOC_MAP_NORMAL];
			extraIndex++;
		}

		// TODO, process the otherMaps?

		// count the meshes with this material
		newGroup->meshCount = 0;
		for (int m = 0; m < raylibModel.meshCount; m++)
		{
			if (raylibModel.meshMaterial[m] == groupIndex)
				newGroup->meshCount++;
		}
		newGroup->meshes = (rlmMesh*)MemAlloc(sizeof(rlmMesh) * newGroup->meshCount);
		newGroup->meshDisableFlags = (bool*)MemAlloc(sizeof(bool) * newGroup->meshCount);

		int meshIndex = 0;
		for (int m = 0; m < raylibModel.meshCount; m++)
		{
			if (raylibModel.meshMaterial[m] != groupIndex)
				continue;

			newGroup->meshDisableFlags[meshIndex] = false;

			Mesh* oldMesh = raylibModel.meshes + m;

			rlmMesh* newMesh = newGroup->meshes + meshIndex;

			newMesh->name = (char*)MemAlloc(32);
			newMesh->name[0] = '\0';
			sprintf(newMesh->name, "imported_mesh_%d", m);

			newMesh->transform = rlmPQSIdentity();

			newMesh->bounds = GetMeshBoundingBox(*oldMesh);

			newMesh->gpuMesh.vaoId = oldMesh->vaoId;
			newMesh->gpuMesh.vboIds = oldMesh->vboId;

			if (oldMesh->indices)
			{
				newMesh->gpuMesh.isIndexed = true;
				newMesh->gpuMesh.elementCount = oldMesh->triangleCount * 3;
			}
			else
			{
				newMesh->gpuMesh.isIndexed = false;
				newMesh->gpuMesh.elementCount = oldMesh->vertexCount;
			}

			if (keepCPUdata)
			{
				newMesh->meshBuffers = (rlmMeshBuffers*)MemAlloc(sizeof(rlmMeshBuffers));
				newMesh->meshBuffers->vertices = oldMesh->vertices;
				newMesh->meshBuffers->texcoords = oldMesh->texcoords;
				newMesh->meshBuffers->normals = oldMesh->normals;
				newMesh->meshBuffers->texcoords2 = oldMesh->texcoords2;
				newMesh->meshBuffers->colors = oldMesh->colors;
				newMesh->meshBuffers->indices = oldMesh->indices;
				newMesh->meshBuffers->boneIds = oldMesh->boneIndices;
				newMesh->meshBuffers->boneWeights = oldMesh->boneWeights;

				newMesh->meshBuffers->vertexCount = oldMesh->vertexCount;
				newMesh->meshBuffers->triangleCount = oldMesh->triangleCount;
			}
			else
			{
				MemFree(oldMesh->vertices);
				MemFree(oldMesh->texcoords);
				MemFree(oldMesh->texcoords2);
				MemFree(oldMesh->normals);
				MemFree(oldMesh->tangents);
				MemFree(oldMesh->colors);
				MemFree(oldMesh->indices);
				MemFree(oldMesh->animVertices);
				MemFree(oldMesh->animNormals);
				MemFree(oldMesh->boneIndices);
				MemFree(oldMesh->boneWeights);
			}

			meshIndex++;
		}
	}

	MemFree(raylibModel.materials);
	MemFree(raylibModel.meshMaterial);
	MemFree(raylibModel.meshes);

	return newModel;
}

rlmModelAniamtionSequence* rlmLoadModelAnimations(rlmSkeleton* skeleton, ModelAnimation* animations, int animationCount)
{
	rlmModelAniamtionSequence* sequences = (rlmModelAniamtionSequence*)MemAlloc(sizeof(rlmModelAniamtionSequence) * animationCount);

	for (int i = 0; i < animationCount; i++)
	{
		memcpy(sequences[i].name, animations[i].name, 32);
		sequences[i].fps = 30;
		sequences[i].keyframeCount = animations[i].keyframeCount;
		sequences[i].keyframes = (rlmAnimationKeyframe*)MemAlloc(sizeof(rlmAnimationKeyframe) * animations[i].keyframeCount);

		for (int f = 0; f < animations[i].keyframeCount; f++)
		{
			sequences[i].keyframes[f].boneTransforms = (rlmPQSTransorm*)MemAlloc(sizeof(rlmPQSTransorm) * skeleton->boneCount);

			for (int b = 0; b < skeleton->boneCount; b++)
			{
				sequences[i].keyframes[f].boneTransforms[b].position = animations[i].keyframePoses[f][b].translation;
				sequences[i].keyframes[f].boneTransforms[b].rotation = animations[i].keyframePoses[f][b].rotation;
				sequences[i].keyframes[f].boneTransforms[b].scale = animations[i].keyframePoses[f][b].scale;
			}

			MemFree(animations[i].keyframePoses[f]);
		}
		MemFree(animations[i].keyframePoses);
	}
	MemFree(animations);

	return sequences;
}