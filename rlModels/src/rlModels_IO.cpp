#include "rlModels_IO.h"

#include <stdio.h>
#include <string.h>

rlmModel rlmLoadFromModel(Model raylibModel)
{
    rlmModel newModel = { 0 };

    if (raylibModel.boneCount > 0 && raylibModel.bones != nullptr)
    {
        newModel.ownsSkeleton = true;

        rlmSkeleton* skeleton = (rlmSkeleton*)MemAlloc(sizeof(rlmSkeleton));

        skeleton->boneCount = raylibModel.boneCount;
        skeleton->bones = (rlmBoneInfo*)MemAlloc(sizeof(rlmBoneInfo) * skeleton->boneCount);

        skeleton->bindingFrame.boneTransforms = (rlmPQSTransorm*)MemAlloc(sizeof(rlmPQSTransorm) * skeleton->boneCount);

        for (int i = 0; i < skeleton->boneCount; i++)
        {
            skeleton->bones[i].boneId = i;
            skeleton->bones[i].name[0] = '\0';
            strcpy(skeleton->bones[i].name, raylibModel.bones[i].name);

            skeleton->bones[i].parentId = raylibModel.bones[i].parent;
            skeleton->bones[i].childBones = NULL;
            skeleton->bones[i].childCount = 0;

            skeleton->bindingFrame.boneTransforms[i].position = raylibModel.bindPose[i].translation;
            skeleton->bindingFrame.boneTransforms[i].scale = raylibModel.bindPose[i].scale;
            skeleton->bindingFrame.boneTransforms[i].rotation = raylibModel.bindPose[i].rotation;
        }

        for (int i = 0; i < skeleton->boneCount; i++)
        {
            if (skeleton->bones[i].parentId < 0)
            {
                skeleton->rootBone = &skeleton->bones[i];
            }
            else
            {
                rlmBoneInfo* parentBone = &skeleton->bones[skeleton->bones[i].parentId];
                parentBone->childCount++;
                parentBone->childBones = (rlmBoneInfo**)MemRealloc(parentBone->childBones, sizeof(rlmBoneInfo*) * parentBone->childCount);
                parentBone->childBones[parentBone->childCount - 1] = &skeleton->bones[i];
            }
        }

        // TODO, make children relative

        newModel.skeleton = skeleton;
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
        newGroup->material.baseChannel.ownsTexture = true;
        newGroup->material.baseChannel.textureLoc = -1;

        newGroup->material.baseChannel.color= raylibModel.materials[groupIndex].maps[MATERIAL_MAP_DIFFUSE].color;

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
            newGroup->material.extraChannels[extraIndex].ownsTexture = true;
            newGroup->material.extraChannels[extraIndex].textureLoc = newGroup->material.shader.locs[SHADER_LOC_MAP_METALNESS];

            newGroup->material.extraChannels[extraIndex].color = raylibModel.materials[groupIndex].maps[SHADER_LOC_MAP_METALNESS].color;
            newGroup->material.extraChannels[extraIndex].colorLoc = newGroup->material.shader.locs[SHADER_LOC_COLOR_SPECULAR];

            extraIndex++;
        }

        if (raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id > 0)
        {
            newGroup->material.extraChannels[extraIndex].cubeMap = false;
            newGroup->material.extraChannels[extraIndex].textureId = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id;
            newGroup->material.extraChannels[extraIndex].textureSlot = 2;
            newGroup->material.extraChannels[extraIndex].ownsTexture = true;
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

            MemFree(oldMesh->vertices);
            MemFree(oldMesh->texcoords);
            MemFree(oldMesh->texcoords2);
            MemFree(oldMesh->normals);
            MemFree(oldMesh->tangents);
            MemFree(oldMesh->colors);
            MemFree(oldMesh->indices);
            MemFree(oldMesh->animVertices);
            MemFree(oldMesh->animNormals);
            MemFree(oldMesh->boneIds);
            MemFree(oldMesh->boneWeights);

            meshIndex++;
        }
    }

    MemFree(raylibModel.bindPose);
    MemFree(raylibModel.bones);
    MemFree(raylibModel.materials);
    MemFree(raylibModel.meshMaterial);
    MemFree(raylibModel.meshes);

    return newModel;
}