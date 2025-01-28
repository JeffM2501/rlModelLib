#include "rlModels_IO.h"

#include <stdio.h>

rlmModel rlmLoadFromModel(Model raylibModel)
{
    rlmModel newModel = { 0 };

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
        newGroup->material.baseChannel.shaderLoc = -1;

        newGroup->material.tint = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_DIFFUSE].color;

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
            newGroup->material.extraChannels[extraIndex].shaderLoc = newGroup->material.shader.locs[SHADER_LOC_MAP_METALNESS];
            extraIndex++;
        }

        if (raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id > 0)
        {
            newGroup->material.extraChannels[extraIndex].cubeMap = false;
            newGroup->material.extraChannels[extraIndex].textureId = raylibModel.materials[groupIndex].maps[MATERIAL_MAP_NORMAL].texture.id;
            newGroup->material.extraChannels[extraIndex].textureSlot = 2;
            newGroup->material.extraChannels[extraIndex].ownsTexture = true;
            newGroup->material.extraChannels[extraIndex].shaderLoc = newGroup->material.shader.locs[SHADER_LOC_MAP_NORMAL];
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

        int meshIndex = 0;
        for (int m = 0; m < raylibModel.meshCount; m++)
        {
            if (raylibModel.meshMaterial[m] != groupIndex)
                continue;

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