#include "rlModels.h"
#include "config.h"
#include "rlgl.h"

#include <string.h>
#include <ctype.h>

static Shader DefaultMaterialShader = { 0 };
static bool DefaultMaterialShaderSet = false;

#define MAX_BONE_NUM 128
static Matrix DefaultBoneMatricies[MAX_BONE_NUM] = { 0 };

#define MAX_MESH_VERTEX_BUFFERS  9      // Maximum vertex buffers (VBO) per mesh

static void CheckGlobalBoneMatricies()
{
    if (DefaultBoneMatricies[0].m0 != 0)
        return;

    for (int i = 0; i < MAX_BONE_NUM; i++)
        DefaultBoneMatricies[i] = MatrixIdentity();
}

void rlmSetDefaultMaterialShader(Shader shader)
{
    DefaultMaterialShader = shader;
    DefaultMaterialShaderSet = true;
}

void rlmClearDefaultMaterialShader()
{
    DefaultMaterialShader = (Shader){ 0 };
    DefaultMaterialShaderSet = false;
}

static void rlUnloadMeshBuffer(rlmMeshBuffers* buffers)
{
    if (!buffers)
        return;

    MemFree(buffers->vertices);
    MemFree(buffers->texcoords);
    MemFree(buffers->texcoords2);
    MemFree(buffers->normals);
    MemFree(buffers->tangents);
    MemFree(buffers->colors);
    MemFree(buffers->indices);
    MemFree(buffers->boneIds);
    MemFree(buffers->boneWeights);

    MemFree(buffers);
}

void rlmUploadMesh(rlmMesh* mesh, bool releaseGeoBuffers)
{
    if (mesh->gpuMesh.vaoId > 0)
    {
        // Check if mesh has already been loaded in GPU
        TraceLog(LOG_WARNING, "VAO: [ID %i] Trying to re-load an already loaded mesh", mesh->gpuMesh.vaoId);
        return;
    }

    if (mesh->meshBuffers == NULL)
    {
        // Check if mesh has already been loaded in GPU
        TraceLog(LOG_WARNING, "Trying to upload a mesh with no mesh buffers");
        return;
    }

    mesh->gpuMesh.vboIds = (unsigned int*)MemAlloc(MAX_MESH_VERTEX_BUFFERS * sizeof(unsigned int));

    mesh->gpuMesh.vaoId = 0;        // Vertex Array Object
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION] = 0;     // Vertex buffer: positions
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD] = 0;     // Vertex buffer: texcoords
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL] = 0;       // Vertex buffer: normals
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR] = 0;        // Vertex buffer: colors
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT] = 0;      // Vertex buffer: tangents
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2] = 0;    // Vertex buffer: texcoords2
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES] = 0;      // Vertex buffer: indices

    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES] = 0;      // Vertex buffer: boneIds
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS] = 0;  // Vertex buffer: boneWeights

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    mesh->gpuMesh.vaoId = rlLoadVertexArray();
    rlEnableVertexArray(mesh->gpuMesh.vaoId);

    // NOTE: Vertex attributes must be uploaded considering default locations points and available vertex data

    bool dynamic = !releaseGeoBuffers;

    // Enable vertex attributes: position (shader-location = 0)
    void* vertices = mesh->meshBuffers->vertices;
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION] = rlLoadVertexBuffer(vertices, mesh->meshBuffers->vertexCount * 3 * sizeof(float), dynamic);
    rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION, 3, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION);

    // Enable vertex attributes: texcoords (shader-location = 1)
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD] = rlLoadVertexBuffer(mesh->meshBuffers->texcoords, mesh->meshBuffers->vertexCount * 2 * sizeof(float), dynamic);
    rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD, 2, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD);

    // WARNING: When setting default vertex attribute values, the values for each generic vertex attribute
    // is part of current state, and it is maintained even if a different program object is used

    if (mesh->meshBuffers->normals != NULL)
    {
        // Enable vertex attributes: normals (shader-location = 2)
        void* normals = mesh->meshBuffers->normals;
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL] = rlLoadVertexBuffer(normals, mesh->meshBuffers->vertexCount * 3 * sizeof(float), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL, 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL);
    }
    else
    {
        // Default vertex attribute: normal
        // WARNING: Default value provided to shader if location available
        float value[3] = { 1.0f, 1.0f, 1.0f };
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL, value, SHADER_ATTRIB_VEC3, 3);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL);
    }

    if (mesh->meshBuffers->colors != NULL)
    {
        // Enable vertex attribute: color (shader-location = 3)
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR] = rlLoadVertexBuffer(mesh->meshBuffers->colors, mesh->meshBuffers->vertexCount * 4 * sizeof(unsigned char), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR, 4, RL_UNSIGNED_BYTE, 1, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR);
    }
    else
    {
        // Default vertex attribute: color
        // WARNING: Default value provided to shader if location available
        float value[4] = { 1.0f, 1.0f, 1.0f, 1.0f };    // WHITE
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR, value, SHADER_ATTRIB_VEC4, 4);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR);
    }

    if (mesh->meshBuffers->tangents != NULL)
    {
        // Enable vertex attribute: tangent (shader-location = 4)
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT] = rlLoadVertexBuffer(mesh->meshBuffers->tangents, mesh->meshBuffers->vertexCount * 4 * sizeof(float), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT, 4, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT);
    }
    else
    {
        // Default vertex attribute: tangent
        // WARNING: Default value provided to shader if location available
        float value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT, value, SHADER_ATTRIB_VEC4, 4);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT);
    }

    if (mesh->meshBuffers->texcoords2 != NULL)
    {
        // Enable vertex attribute: texcoord2 (shader-location = 5)
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2] = rlLoadVertexBuffer(mesh->meshBuffers->texcoords2, mesh->meshBuffers->vertexCount * 2 * sizeof(float), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2, 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2);
    }
    else
    {
        // Default vertex attribute: texcoord2
        // WARNING: Default value provided to shader if location available
        float value[2] = { 0.0f, 0.0f };
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2, value, SHADER_ATTRIB_VEC2, 2);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2);
    }

    if (mesh->meshBuffers->boneIds != NULL)
    {
        // Enable vertex attribute: boneIds (shader-location = 7)
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES] = rlLoadVertexBuffer(mesh->meshBuffers->boneIds, mesh->meshBuffers->vertexCount * 4 * sizeof(unsigned char), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES, 4, RL_UNSIGNED_BYTE, 0, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES);
    }
    else
    {
        // Default vertex attribute: boneIds
        // WARNING: Default value provided to shader if location available
        float value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES, value, SHADER_ATTRIB_VEC4, 4);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES);
    }

    if (mesh->meshBuffers->boneWeights != NULL)
    {
        // Enable vertex attribute: boneWeights (shader-location = 8)
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS] = rlLoadVertexBuffer(mesh->meshBuffers->boneWeights, mesh->meshBuffers->vertexCount * 4 * sizeof(float), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS, 4, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS);
    }
    else
    {
        // Default vertex attribute: boneWeights
        // WARNING: Default value provided to shader if location available
        float value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS, value, SHADER_ATTRIB_VEC4, 2);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS);
    }

    if (mesh->meshBuffers->indices != NULL)
    {
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES] = rlLoadVertexBufferElement(mesh->meshBuffers->indices, mesh->meshBuffers->triangleCount * 3 * sizeof(unsigned short), dynamic);
    }

    if (mesh->gpuMesh.vaoId > 0) TRACELOG(LOG_INFO, "VAO: [ID %i] Mesh uploaded successfully to VRAM (GPU)", mesh->gpuMesh.vaoId);
    else TRACELOG(LOG_INFO, "VBO: Mesh uploaded successfully to VRAM (GPU)");

    rlDisableVertexArray();
#endif

    if (releaseGeoBuffers)
    {
        rlUnloadMeshBuffer(mesh->meshBuffers);
        mesh->meshBuffers = NULL;
    }
}

void rlmUnloadMesh(rlmMesh* mesh)
{
    if (!mesh)
        return;

    MemFree(mesh->name);
    mesh->name = NULL;

    rlUnloadVertexArray(mesh->gpuMesh.vaoId);

    if (mesh->gpuMesh.vboIds != NULL)
    {
        for (int i = 0; i < MAX_MESH_VERTEX_BUFFERS; i++)
            rlUnloadVertexBuffer(mesh->gpuMesh.vboIds[i]);
    }
    MemFree(mesh->gpuMesh.vboIds);

    mesh->gpuMesh.vaoId = 0;
    mesh->gpuMesh.vboIds = NULL;

    rlUnloadMeshBuffer(mesh->meshBuffers);
}

rlmMaterialDef rlmGetDefaultMaterial()
{
    rlmMaterialDef material = { 0 };
    material.name = (char*)MemAlloc(32);
    material.name[0] = '\0';
    strcpy(material.name, "rlmDefaultMaterial");

    if (DefaultMaterialShaderSet)
    {
        material.shader.id = DefaultMaterialShader.id;
        material.shader.locs = DefaultMaterialShader.locs;
        material.ownsShader = false;
    }
    else
    {
        material.shader.id = rlGetShaderIdDefault();
        material.shader.locs = rlGetShaderLocsDefault();
        material.ownsShader = true;
    }

    material.baseChannel.ownsTexture = false;
    material.baseChannel.textureId = rlGetTextureIdDefault();
    material.baseChannel.textureLoc = -1;

    material.baseChannel.color = WHITE;
    material.baseChannel.colorLoc = -SHADER_LOC_COLOR_DIFFUSE;

    material.extraChannels = 0;
    material.extraChannels = NULL;

    return material;
}

void rlmUnloadMaterial(rlmMaterialDef* material)
{
    if (!material)
        return;

    MemFree(material->name);
    material->name = NULL;

    if (material->ownsShader)
    {
        if (material->shader.id != rlGetShaderIdDefault())
        {
            rlUnloadShaderProgram(material->shader.id);
            MemFree(material->shader.locs);
        }
        material->shader.locs = NULL;
    }

    if (material->baseChannel.ownsTexture)
    {
        rlUnloadTexture(material->baseChannel.textureId);
        material->baseChannel.textureId = 0;
    }

    for (int i = 0; i < material->materialChannels; i++)
    {
        if (material->extraChannels[i].ownsTexture)
            rlUnloadTexture(material->extraChannels[i].textureId);
        material->extraChannels[i].textureId = 0;
    }

    if (material->values)
        MemFree(material->values);

    material->values = NULL;
    material->materialValues = 0;
}

void rlmAddMaterialChannel(rlmMaterialDef* material, Texture2D texture, int shaderLoc, int slot, bool isCubeMap)
{
    if (!material)
        return;

    if (material->materialChannels == 0 || !material->extraChannels)
    {
        material->materialChannels = 1;
        material->extraChannels = (rlmMaterialChannel*)MemAlloc(sizeof(rlmMaterialChannel));
    }
    else
    {
        material->materialChannels++;
        material->extraChannels = (rlmMaterialChannel*)MemRealloc(material->extraChannels, sizeof(rlmMaterialChannel) * material->materialChannels);
    }

    material->extraChannels[material->materialChannels - 1].ownsTexture = false;
    material->extraChannels[material->materialChannels - 1].cubeMap = isCubeMap;
    material->extraChannels[material->materialChannels - 1].textureId = texture.id;
    material->extraChannels[material->materialChannels - 1].textureLoc = shaderLoc;
    material->extraChannels[material->materialChannels - 1].textureSlot = slot;
    material->extraChannels[material->materialChannels - 1].color = WHITE;
    material->extraChannels[material->materialChannels - 1].colorLoc = 0;
}

void rlmAddMaterialChannels(rlmMaterialDef* material, int count, Texture2D* textures, int* locs)
{
    if (!material)
        return;

    if (material->materialChannels == 0 || !material->extraChannels)
    {
        material->materialChannels = count;
        material->extraChannels = (rlmMaterialChannel*)MemAlloc(sizeof(rlmMaterialChannel) * count);
    }
    else
    {
        material->materialChannels += count;
        material->extraChannels = (rlmMaterialChannel*)MemRealloc(material->extraChannels, sizeof(rlmMaterialChannel) * material->materialChannels);
    }

    for (int i = 0; i < count; i++)
    {
        int index = material->materialChannels - count + i;

        material->extraChannels[material->materialChannels - 1].cubeMap = false;
        material->extraChannels[material->materialChannels - 1].ownsTexture = false;

        material->extraChannels[material->materialChannels - 1].color = WHITE;
        material->extraChannels[material->materialChannels - 1].colorLoc = -1;

        material->extraChannels[material->materialChannels - 1].textureSlot = -1;

        if (textures && locs)
        {
            material->extraChannels[material->materialChannels - 1].textureId = textures[i].id;
            material->extraChannels[material->materialChannels - 1].textureLoc = locs[i];
        }
        else
        {
            material->extraChannels[material->materialChannels - 1].textureId = 0;
            material->extraChannels[material->materialChannels - 1].textureLoc = -1;
        }
    }
}

int rlmAddMaterialValue(rlmMaterialDef* material, int shaderLoc, float value)
{
    if (!material)
        return -1;

    int index = 0;
    if (!material->values || material->materialValues == 0)
    {
        material->values = (rlmMaterialValueF*)MemAlloc(sizeof(rlmMaterialValueF));
        material->materialValues = 1;
    }
    else
    {
        index = material->materialValues;
        material->materialValues++;
        material->values = (rlmMaterialValueF*)MemRealloc(material->values, sizeof(rlmMaterialValueF) * material->materialValues);
    }

    material->values[index].shaderLoc = shaderLoc;
    material->values[index].value = value;

    return index;
}

void rlmSetMaterialValue(rlmMaterialDef* material, int valueIndex, float value)
{
    if (!material || valueIndex >= material->materialValues)
        return;

    material->values[valueIndex].value = value;
}

void rlmSetMaterialDefShader(rlmMaterialDef* material, Shader shader)
{
    if (!material)
        return;

    material->shader = shader;
    material->ownsShader = false;
}

void rlmSetMaterialChannelTexture(rlmMaterialChannel* channel, Texture2D texture)
{
    if (!channel)
        return;

    channel->ownsTexture = false;
    channel->textureId = texture.id;
}

void rlmCloneMaterial(const rlmMaterialDef* oldMaterial, rlmMaterialDef* newMaterial)
{
    if (!oldMaterial || !newMaterial)
        return;

    newMaterial->name = (char*)MemAlloc(32);
    newMaterial->name[0] = '\0';

    strcpy(newMaterial->name, oldMaterial->name);
    newMaterial->baseChannel = oldMaterial->baseChannel;
    newMaterial->baseChannel.ownsTexture = false;

    newMaterial->shader = oldMaterial->shader;
    newMaterial->ownsShader = false;

    newMaterial->materialChannels = oldMaterial->materialChannels;

    if (newMaterial->materialChannels > 0 && oldMaterial->extraChannels)
    {
        newMaterial->extraChannels = (rlmMaterialChannel*)MemAlloc(sizeof(rlmMaterialChannel) * newMaterial->materialChannels);

        for (int i = 0; i < newMaterial->materialChannels; i++)
        {
            newMaterial->extraChannels[i] = oldMaterial->extraChannels[i];
            newMaterial->extraChannels[i].ownsTexture = false;
        }
    }

    newMaterial->materialValues = oldMaterial->materialValues;

    if (newMaterial->materialValues > 0 && oldMaterial->materialValues)
    {
        newMaterial->values = (rlmMaterialValueF*)MemAlloc(sizeof(rlmMaterialValueF) * newMaterial->materialValues);

        for (int i = 0; i < newMaterial->materialValues; i++)
        {
            newMaterial->values[i].shaderLoc = oldMaterial->values[i].shaderLoc;
            newMaterial->values[i].value = oldMaterial->values[i].value;
        }
    }
}

rlmModel rlmCloneModel(rlmModel model)
{
    rlmModel newModel;

    newModel.orientationTransform = model.orientationTransform;

    newModel.groupCount = model.groupCount;

    newModel.skeleton = model.skeleton;
    newModel.ownsSkeleton = false;

    newModel.groups = (rlmModelGroup*)MemAlloc(sizeof(rlmModelGroup) * newModel.groupCount);
    for (int group = 0; group < newModel.groupCount; group++)
    {
        rlmModelGroup* newGroup = newModel.groups + group;
        const rlmModelGroup* oldGroup = model.groups + group;

        rlmCloneMaterial(&oldGroup->material, &newGroup->material);

        newGroup->ownsMeshes = false;
        newGroup->ownsMeshList = false;

        newGroup->meshDisableFlags = (bool*)MemAlloc(sizeof(bool) * oldGroup->meshCount);
        for (int i = 0; i < oldGroup->meshCount; i++)
            newGroup->meshDisableFlags[i] = oldGroup->meshDisableFlags[i];

        newGroup->meshCount = oldGroup->meshCount;
        newGroup->meshes = oldGroup->meshes;
    }

    return newModel;
}

void rlmSetModelShader(rlmModel* model, Shader shader)
{
    for (int group = 0; group < model->groupCount; group++)
    {
        rlmModelGroup* groupPtr = model->groups + group;

        groupPtr->material.shader = shader;
        groupPtr->material.ownsShader = false;
    }
}

void rlmUnloadModel(rlmModel* model)
{
    if (!model)
        return;

    for (int group = 0; group < model->groupCount; group++)
    {
        rlmModelGroup* groupPtr = model->groups + group;

        rlmUnloadMaterial(&groupPtr->material);

        if (groupPtr->ownsMeshes)
        {
            for (int i = 0; i < groupPtr->meshCount; i++)
                rlmUnloadMesh(groupPtr->meshes + i);
        }

        if (groupPtr->ownsMeshList)
            MemFree(groupPtr->meshes);

        MemFree(groupPtr->meshDisableFlags);

        groupPtr->meshDisableFlags = NULL;
        groupPtr->meshCount = 0;
        groupPtr->meshes = 0;
    }

    model->groupCount = 0;
    MemFree(model->groups);
    model->groups = NULL;

    if (model->ownsSkeleton && model->skeleton)
        rlmUnloadSkeleton(model->skeleton);

    model->ownsSkeleton = false;
    model->skeleton = NULL;
}

void rlmApplyMaterialChannel(rlmMaterialChannel* channel, Shader* shader, int index)
{
    if (!channel)
        return;

    rlActiveTextureSlot(channel->textureSlot);
    if (channel->cubeMap)
        rlEnableTextureCubemap(channel->textureId);
    else
        rlEnableTexture(channel->textureId);
    rlSetUniform(channel->textureLoc, &channel->textureSlot, SHADER_UNIFORM_INT, 1);

    int locToUse = channel->colorLoc;
    if (locToUse < 0)
        locToUse = shader->locs[locToUse * -1];

    // if (locToUse > 0)
    {
        float values[4] = {
               (float)channel->color.r / 255.0f,
               (float)channel->color.g / 255.0f,
               (float)channel->color.b / 255.0f,
               (float)channel->color.a / 255.0f
        };
        rlSetUniform(locToUse, values, SHADER_UNIFORM_VEC4, 1);
    }
}

void rlmResetMaterialChannel(rlmMaterialChannel* channel)
{
    if (!channel)
        return;

    rlActiveTextureSlot(channel->textureSlot);
    if (channel->cubeMap)
        rlDisableTextureCubemap();
    else
        rlDisableTexture();
}

void rlmApplyMaterialDef(rlmMaterialDef* material)
{
    if (!material)
        return;

    rlEnableShader(material->shader.id);

    int index = 0;

    rlmApplyMaterialChannel(&material->baseChannel, &material->shader, 0);

    for (index = 1; index < material->materialChannels + 1; index++)
        rlmApplyMaterialChannel(&material->extraChannels[index - 1], &material->shader, 0);

    for (int i = 0; i < material->materialValues; i++)
        SetShaderValue(material->shader, material->values[i].shaderLoc, &material->values[i].value, SHADER_UNIFORM_FLOAT);
}

void rlmResetMaterialDef(rlmMaterialDef* material)
{
    if (!material)
        return;

    int index = 0;
    rlmResetMaterialChannel(&material->baseChannel);

    for (index = 1; index < material->materialChannels + 1; index++)
        rlmResetMaterialChannel(&material->extraChannels[index - 1]);
}

Matrix rlmPQSToMatrix(const rlmPQSTransorm* transform)
{
    Matrix matScale = MatrixScale(transform->scale.x, transform->scale.y, transform->scale.z);

    Vector3 axis = { 0 };
    float angle = 0;

    QuaternionToAxisAngle(transform->rotation, &axis, &angle);
    Matrix matRotation = MatrixRotate(axis, angle);
    Matrix matTranslation = MatrixTranslate(transform->position.x, transform->position.y, transform->position.z);

    return MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
}

rlmPQSTransorm rlmPQSFromMatrix(Matrix matrix)
{
    rlmPQSTransorm transform;

    MatrixDecompose(matrix, &transform.position, &transform.rotation, &transform.scale);

    return transform;
}

void rlmDrawMesh(rlmGPUMesh* mesh, Shader* shader)
{
    if (!mesh || !shader)
        return;

    // Try binding vertex array objects (VAO) or use VBOs if not possible
    // WARNING: UploadMesh() enables all vertex attributes available in mesh and sets default attribute values
    // for shader expected vertex attributes that are not provided by the mesh (i.e. colors)
    // This could be a dangerous approach because different meshes with different shaders can enable/disable some attributes
    if (!rlEnableVertexArray(mesh->vaoId))
    {
        // Bind mesh VBO data: vertex position (shader-location = 0)
        rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION]);

        // Bind mesh VBO data: vertex texcoords (shader-location = 1)
        rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD]);
        rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_TEXCOORD01], 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_TEXCOORD01]);

        if (shader->locs[SHADER_LOC_VERTEX_NORMAL] != -1)
        {
            // Bind mesh VBO data: vertex normals (shader-location = 2)
            rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL]);
            rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_NORMAL]);
        }

        // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
        if (shader->locs[SHADER_LOC_VERTEX_COLOR] != -1)
        {
            if (mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR] != 0)
            {
                rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR]);
                rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_COLOR], 4, RL_UNSIGNED_BYTE, 1, 0, 0);
                rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_COLOR]);
            }
            else
            {
                // Set default value for defined vertex attribute in shader but not provided by mesh
                // WARNING: It could result in GPU undefined behavior
                static float value[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                rlSetVertexAttributeDefault(shader->locs[SHADER_LOC_VERTEX_COLOR], value, SHADER_ATTRIB_VEC4, 4);
                rlDisableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_COLOR]);
            }
        }

        // Bind mesh VBO data: vertex tangents (shader-location = 4, if available)
        if (shader->locs[SHADER_LOC_VERTEX_TANGENT] != -1)
        {
            rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT]);
            rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_TANGENT], 4, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_TANGENT]);
        }

        // Bind mesh VBO data: vertex texcoords2 (shader-location = 5, if available)
        if (shader->locs[SHADER_LOC_VERTEX_TEXCOORD02] != -1)
        {
            rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2]);
            rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_TEXCOORD02], 2, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_TEXCOORD02]);
        }

        // Bind mesh VBO data: vertex bone ids (shader-location = 6, if available)
        if (shader->locs[SHADER_LOC_VERTEX_BONEIDS] != -1)
        {
            rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES]);
            rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_BONEIDS], 4, RL_UNSIGNED_BYTE, 0, 0, 0);
            rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_BONEIDS]);
        }

        // Bind mesh VBO data: vertex bone weights (shader-location = 7, if available)
        if (shader->locs[SHADER_LOC_VERTEX_BONEWEIGHTS] != -1)
        {
            rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS]);
            rlSetVertexAttribute(shader->locs[SHADER_LOC_VERTEX_BONEWEIGHTS], 4, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(shader->locs[SHADER_LOC_VERTEX_BONEWEIGHTS]);
        }

        if (mesh->isIndexed)
            rlEnableVertexBufferElement(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES]);
    }

    // Draw mesh
    if (mesh->isIndexed)
        rlDrawVertexArrayElements(0, mesh->elementCount, 0);
    else
        rlDrawVertexArray(0, mesh->elementCount);

    // Disable all possible vertex array objects (or VBOs)
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();
}

void rlmDrawModel(rlmModel model, rlmPQSTransorm transform)
{
    Matrix transformMatrix = rlmPQSToMatrix(&transform);
    Matrix modelMatrix = MatrixMultiply(rlmPQSToMatrix(&model.orientationTransform), transformMatrix);

    // add in the rlgl matrix stack
    Matrix matModel = MatrixMultiply(modelMatrix, rlGetMatrixTransform());

    for (int group = 0; group < model.groupCount; group++)
    {
        rlmModelGroup* groupPtr = model.groups + group;

        rlmApplyMaterialDef(&groupPtr->material);

        // draw the meshes
        for (int i = 0; i < groupPtr->meshCount; i++)
        {
            // load uniforms
            // Get a copy of current matrices to work with,
            // just in case stereo render is required, and we need to modify them
            // NOTE: At this point the modelview matrix just contains the view matrix (camera)
            // That's because BeginMode3D() sets it and there is no model-drawing function
            // that modifies it, all use rlPushMatrix() and rlPopMatrix()
            Matrix matView = MatrixMultiply(rlmPQSToMatrix(&groupPtr->meshes[i].transform), rlGetMatrixModelview());
            Matrix matProjection = rlGetMatrixProjection();
            Matrix matModelView = MatrixMultiply(matModel, matView);
            Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);

            // Upload view and projection matrices (if locations available)
            if (groupPtr->material.shader.locs[SHADER_LOC_MATRIX_VIEW] != -1)
                rlSetUniformMatrix(groupPtr->material.shader.locs[SHADER_LOC_MATRIX_VIEW], matView);

            if (groupPtr->material.shader.locs[SHADER_LOC_MATRIX_PROJECTION] != -1)
                rlSetUniformMatrix(groupPtr->material.shader.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);

            // Model transformation matrix is sent to shader uniform location: SHADER_LOC_MATRIX_MODEL
            if (groupPtr->material.shader.locs[SHADER_LOC_MATRIX_MODEL] != -1)
                rlSetUniformMatrix(groupPtr->material.shader.locs[SHADER_LOC_MATRIX_MODEL], matModel);

            // Upload model normal matrix (if locations available)
            if (groupPtr->material.shader.locs[SHADER_LOC_MATRIX_NORMAL] != -1)
                rlSetUniformMatrix(groupPtr->material.shader.locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));

            // Send combined model-view-projection matrix to shader
            rlSetUniformMatrix(groupPtr->material.shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

            // if the shader wants bones, set identity matricies, so it can still draw
            if (groupPtr->material.shader.locs[SHADER_LOC_MATRIX_BONETRANSFORMS] >= 0)
            {
                int count = MAX_BONE_NUM;
                if (model.skeleton)
                    count = model.skeleton->boneCount;

                CheckGlobalBoneMatricies();
                rlSetUniformMatrices(groupPtr->material.shader.locs[SHADER_LOC_MATRIX_BONETRANSFORMS], DefaultBoneMatricies, count);
            }

            if (groupPtr->meshDisableFlags == NULL || !groupPtr->meshDisableFlags[i])
                rlmDrawMesh(&groupPtr->meshes[i].gpuMesh, &groupPtr->material.shader);
        }

        rlmResetMaterialDef(&groupPtr->material);
        // Disable shader program
        rlDisableShader();
    }

    rlSetTexture(0);
}

void rlmDrawModelWithPose(rlmModel model, rlmPQSTransorm transform, rlmModelAnimationPose* pose)
{
    rlmDrawModelWithPoseEx(model, transform, pose, NULL);
}

void rlmDrawModelWithPoseEx(rlmModel model, rlmPQSTransorm transform, rlmModelAnimationPose* pose, Shader* shader)
{
    Matrix transformMatrix = rlmPQSToMatrix(&transform);
    Matrix modelMatrix = MatrixMultiply(rlmPQSToMatrix(&model.orientationTransform), transformMatrix);

    // add in the rlgl matrix stack
    Matrix matModel = MatrixMultiply(modelMatrix, rlGetMatrixTransform());

    for (int group = 0; group < model.groupCount; group++)
    {
        rlmModelGroup* groupPtr = model.groups + group;

        Shader* shaderToUse = &groupPtr->material.shader;
        if (shader)
        {
            shaderToUse = shader; // shader override, assume it is already setup
        }
        else
        {
            rlmApplyMaterialDef(&groupPtr->material);
        }

        // if the shader wants bones, set some bone matricies
        if (shaderToUse->locs[SHADER_LOC_MATRIX_BONETRANSFORMS] >= 0)
        {
            // if we have a real pose, use it
            if (model.skeleton && pose)
            {
                rlSetUniformMatrices(shaderToUse->locs[SHADER_LOC_MATRIX_BONETRANSFORMS], pose->boneMatricies, model.skeleton->boneCount);
            }
            else // otherwise just fill out a list of default bones.
            {
                int count = MAX_BONE_NUM;
                if (model.skeleton)
                    count = model.skeleton->boneCount;

                CheckGlobalBoneMatricies();
                rlSetUniformMatrices(shaderToUse->locs[SHADER_LOC_MATRIX_BONETRANSFORMS], DefaultBoneMatricies, count);
            }
        }

        // load uniforms
        // Get a copy of current matrices to work with,
        // just in case stereo render is required, and we need to modify them
        // NOTE: At this point the modelview matrix just contains the view matrix (camera)
        // That's because BeginMode3D() sets it and there is no model-drawing function
        // that modifies it, all use rlPushMatrix() and rlPopMatrix()
        Matrix matView = rlGetMatrixModelview();
        Matrix matProjection = rlGetMatrixProjection();
        Matrix matModelView = MatrixMultiply(matModel, matView);
        Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);

        // Upload view and projection matrices (if locations available)
        if (shaderToUse->locs[SHADER_LOC_MATRIX_VIEW] != -1)
            rlSetUniformMatrix(shaderToUse->locs[SHADER_LOC_MATRIX_VIEW], matView);

        if (shaderToUse->locs[SHADER_LOC_MATRIX_PROJECTION] != -1)
            rlSetUniformMatrix(shaderToUse->locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);

        // Model transformation matrix is sent to shader uniform location: SHADER_LOC_MATRIX_MODEL
        if (shaderToUse->locs[SHADER_LOC_MATRIX_MODEL] != -1)
            rlSetUniformMatrix(shaderToUse->locs[SHADER_LOC_MATRIX_MODEL], matModel);

        // Upload model normal matrix (if locations available)
        if (shaderToUse->locs[SHADER_LOC_MATRIX_NORMAL] != -1)
            rlSetUniformMatrix(shaderToUse->locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));

        // Send combined model-view-projection matrix to shader
        rlSetUniformMatrix(shaderToUse->locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

        // draw the meshes
        for (int i = 0; i < groupPtr->meshCount; i++)
        {
            if (groupPtr->meshDisableFlags == NULL || !groupPtr->meshDisableFlags[i])
                rlmDrawMesh(&groupPtr->meshes[i].gpuMesh, &groupPtr->material.shader);
        }

        if (!shader)
            rlmResetMaterialDef(&groupPtr->material);

        // Disable shader program
        rlDisableShader();
    }

    rlSetTexture(0);
}

Matrix rlmGetBoneMatrix(const rlmPQSTransorm* bindingTransform, const rlmPQSTransorm* frameTransform)
{
    Vector3 invTranslation = Vector3RotateByQuaternion(Vector3Negate(bindingTransform->position), QuaternionInvert(bindingTransform->rotation));
    Quaternion invRotation = QuaternionInvert(bindingTransform->rotation);
    Vector3 invScale = Vector3Divide((Vector3) { 1, 1, 1 }, bindingTransform->scale);

    Vector3 boneTranslation = Vector3Add(Vector3RotateByQuaternion(Vector3Multiply(frameTransform->scale, invTranslation), frameTransform->rotation), frameTransform->position);
    Quaternion boneRotation = QuaternionMultiply(frameTransform->rotation, invRotation);
    Vector3 boneScale = Vector3Multiply(frameTransform->scale, invScale);

    Matrix boneMatrix = MatrixMultiply(MatrixMultiply(
        QuaternionToMatrix(boneRotation),
        MatrixTranslate(boneTranslation.x, boneTranslation.y, boneTranslation.z)),
        MatrixScale(boneScale.x, boneScale.y, boneScale.z));

    return boneMatrix;
}

static void rlmSetBonePoseRecursive(const rlmBoneInfo* bone, const rlmAnimationKeyframe* bindingFrame, const rlmAnimationKeyframe* keyframe, rlmModelAnimationPose* pose)
{
    pose->boneMatricies[bone->boneId] = rlmGetBoneMatrix(&bindingFrame->boneTransforms[bone->boneId], &keyframe->boneTransforms[bone->boneId]);

    for (int i = 0; i < bone->childCount; i++)
    {
        rlmSetBonePoseRecursive(bone->childBones[i], bindingFrame, keyframe, pose);
    }
}

static void rlmSetBonePoseRecursiveLerp(const rlmBoneInfo* bone, const rlmAnimationKeyframe* bindingFrame, const rlmAnimationKeyframe* keyframe1, const rlmAnimationKeyframe* keyframe2, float param, rlmModelAnimationPose* pose)
{
    rlmPQSTransorm lerpedTransform = rlmPQSLerp(&keyframe1->boneTransforms[bone->boneId], &keyframe2->boneTransforms[bone->boneId], param);

    pose->boneMatricies[bone->boneId] = rlmGetBoneMatrix(&bindingFrame->boneTransforms[bone->boneId], &lerpedTransform);

    for (int i = 0; i < bone->childCount; i++)
    {
        rlmSetBonePoseRecursiveLerp(bone->childBones[i], bindingFrame, keyframe1, keyframe2, param, pose);
    }
}

rlmModelAnimationPose rlmLoadPoseFromModel(rlmModel model)
{
    rlmModelAnimationPose pose = { 0 };

    if (model.skeleton)
    {
        pose.boneMatricies = (Matrix*)MemAlloc(sizeof(Matrix) * model.skeleton->boneCount);

        for (int i = 0; i < model.skeleton->boneCount; i++)
            pose.boneMatricies[i] = MatrixIdentity();
    }
    return pose;
}

void rlmUnloadPose(rlmModelAnimationPose* pose)
{
    if (!pose)
        return;

    MemFree(pose->boneMatricies);
    pose->boneMatricies = NULL;
}

void rlmSetPoseToKeyframe(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame)
{
    if (!model.skeleton)
        return;

    rlmSetBonePoseRecursive(model.skeleton->rootBone, &model.skeleton->bindingFrame, &frame, pose);
}

void rlmSetPoseToKeyframeEx(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame, rlmBoneInfo* startBone)
{
    if (!model.skeleton)
        return;

    if (startBone == NULL)
        startBone = model.skeleton->rootBone;

    rlmSetBonePoseRecursive(startBone, &model.skeleton->bindingFrame, &frame, pose);
}

void rlmSetPoseToKeyframesLerp(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame1, rlmAnimationKeyframe frame2, float param)
{
    if (!model.skeleton)
        return;

    rlmSetBonePoseRecursiveLerp(model.skeleton->rootBone, &model.skeleton->bindingFrame, &frame1, &frame2, param, pose);
}

void rlmSetPoseToKeyframesLerpEx(rlmModel model, rlmModelAnimationPose* pose, rlmAnimationKeyframe frame1, rlmAnimationKeyframe frame2, float param, rlmBoneInfo* startBone)
{
    if (!model.skeleton)
        return;

    if (startBone == NULL)
        startBone = model.skeleton->rootBone;

    rlmSetBonePoseRecursiveLerp(startBone, &model.skeleton->bindingFrame, &frame1, &frame2, param, pose);
}

rlmBoneInfo* rlmFindBoneByName(rlmModel model, const char* boneName)
{
    if (!model.skeleton || !boneName)
        return NULL;

    for (int i = 0; i < model.skeleton->boneCount; i++)
    {
        if (TextIsEqual(model.skeleton->bones[i].name, boneName))
            return &model.skeleton->bones[i];
    }
    return NULL;
}

void rlmUnloadSkeleton(rlmSkeleton* skeleton)
{
    if (!skeleton)
        return;

    if (skeleton->bones)
    {
        for (int i = 0; i < skeleton->boneCount; i++)
            MemFree(skeleton->bones[i].childBones);
    }

    rlmUnloadSkeletonRetargetInfo(skeleton);

    MemFree(skeleton->bones);
    MemFree(skeleton->bindingFrame.boneTransforms);
    MemFree(skeleton);
}

static bool rlmNameContainsNoCase(const char* name, const char* token)
{
    size_t tokenLen = strlen(token);
    for (const char* start = name; *start; start++)
    {
        size_t i = 0;
        while (i < tokenLen && start[i] && tolower((unsigned char)start[i]) == tolower((unsigned char)token[i]))
            i++;

        if (i == tokenLen)
            return true;
    }
    return false;
}

static int rlmDetectHipBone(const rlmSkeleton* skeleton)
{
    const rlmSkeletonRetargetInfo* info = skeleton->retargetInfo;

    // evalOrder is shallowest first, so the first match is the one closest to the root
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        const rlmBoneInfo* bone = &skeleton->bones[info->evalOrder[i]];
        if (rlmNameContainsNoCase(bone->name, "hips") || rlmNameContainsNoCase(bone->name, "pelvis"))
            return bone->boneId;
    }

    // no name match, a hip normally has a spine and two legs under it
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        const rlmBoneInfo* bone = &skeleton->bones[info->evalOrder[i]];
        if (bone->childCount >= 3)
            return bone->boneId;
    }

    return skeleton->rootBone ? skeleton->rootBone->boneId : -1;
}

static void rlmUpdateHipHeight(rlmSkeleton* skeleton)
{
    rlmSkeletonRetargetInfo* info = skeleton->retargetInfo;

    // distance from the origin rather than a single axis, so it does not depend on which way is up
    if (info->hipBone >= 0 && info->hipBone < skeleton->boneCount)
        info->hipHeight = Vector3Length(info->referencePose.boneTransforms[info->hipBone].position);
    else
        info->hipHeight = 0;
}

void rlmComputeSkeletonRetargetInfo(rlmSkeleton* skeleton)
{
    if (!skeleton || skeleton->boneCount <= 0 || !skeleton->bones || !skeleton->bindingFrame.boneTransforms)
        return;

    bool existing = skeleton->retargetInfo != NULL;
    if (!existing)
    {
        skeleton->retargetInfo = (rlmSkeletonRetargetInfo*)MemAlloc(sizeof(rlmSkeletonRetargetInfo));
        skeleton->retargetInfo->hipBone = -1;
    }

    rlmSkeletonRetargetInfo* info = skeleton->retargetInfo;
    size_t transformsSize = sizeof(rlmPQSTransorm) * skeleton->boneCount;

    if (!info->localBindingFrame.boneTransforms)
        info->localBindingFrame.boneTransforms = (rlmPQSTransorm*)MemAlloc((unsigned int)transformsSize);

    if (!info->referencePose.boneTransforms)
    {
        info->referencePose.boneTransforms = (rlmPQSTransorm*)MemAlloc((unsigned int)transformsSize);
        memcpy(info->referencePose.boneTransforms, skeleton->bindingFrame.boneTransforms, transformsSize);
    }

    if (!info->localReferencePose.boneTransforms)
        info->localReferencePose.boneTransforms = (rlmPQSTransorm*)MemAlloc((unsigned int)transformsSize);

    if (!info->evalOrder)
        info->evalOrder = (int*)MemAlloc(sizeof(int) * skeleton->boneCount);

    // breadth first from every root, using evalOrder as the queue
    bool* visited = (bool*)MemAlloc(sizeof(bool) * skeleton->boneCount);
    int count = 0;

    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (skeleton->bones[i].parentId < 0)
        {
            info->evalOrder[count++] = i;
            visited[i] = true;
        }
    }

    for (int head = 0; head < count; head++)
    {
        const rlmBoneInfo* bone = &skeleton->bones[info->evalOrder[head]];
        for (int c = 0; c < bone->childCount; c++)
        {
            int childId = bone->childBones[c]->boneId;
            if (!visited[childId])
            {
                info->evalOrder[count++] = childId;
                visited[childId] = true;
            }
        }
    }

    if (count < skeleton->boneCount)
    {
        TraceLog(LOG_WARNING, "rlModels : %d bones are not reachable from a root bone, they are appended to the evaluation order", skeleton->boneCount - count);
        for (int i = 0; i < skeleton->boneCount; i++)
        {
            if (!visited[i])
                info->evalOrder[count++] = i;
        }
    }
    MemFree(visited);

    rlmKeyframeModelToLocal(skeleton, &skeleton->bindingFrame, &info->localBindingFrame);
    rlmKeyframeModelToLocal(skeleton, &info->referencePose, &info->localReferencePose);

    if (!existing || info->hipBone < 0 || info->hipBone >= skeleton->boneCount)
        info->hipBone = rlmDetectHipBone(skeleton);

    rlmUpdateHipHeight(skeleton);
}

void rlmSetSkeletonReferencePose(rlmSkeleton* skeleton, rlmAnimationKeyframe modelSpacePose)
{
    if (!skeleton || !modelSpacePose.boneTransforms)
        return;

    if (!skeleton->retargetInfo)
        rlmComputeSkeletonRetargetInfo(skeleton);

    if (!skeleton->retargetInfo)
        return;

    memcpy(skeleton->retargetInfo->referencePose.boneTransforms, modelSpacePose.boneTransforms, sizeof(rlmPQSTransorm) * skeleton->boneCount);
    rlmKeyframeModelToLocal(skeleton, &skeleton->retargetInfo->referencePose, &skeleton->retargetInfo->localReferencePose);
    rlmUpdateHipHeight(skeleton);
}

void rlmSetSkeletonHipBone(rlmSkeleton* skeleton, int boneId)
{
    if (!skeleton || boneId < -1 || boneId >= skeleton->boneCount)
        return;

    if (!skeleton->retargetInfo)
        rlmComputeSkeletonRetargetInfo(skeleton);

    if (!skeleton->retargetInfo)
        return;

    skeleton->retargetInfo->hipBone = boneId;
    rlmUpdateHipHeight(skeleton);
}

void rlmUnloadSkeletonRetargetInfo(rlmSkeleton* skeleton)
{
    if (!skeleton || !skeleton->retargetInfo)
        return;

    MemFree(skeleton->retargetInfo->localBindingFrame.boneTransforms);
    MemFree(skeleton->retargetInfo->referencePose.boneTransforms);
    MemFree(skeleton->retargetInfo->localReferencePose.boneTransforms);
    MemFree(skeleton->retargetInfo->evalOrder);
    MemFree(skeleton->retargetInfo);
    skeleton->retargetInfo = NULL;
}

#define RLM_BONE_KEY_SIZE 48

static bool rlmIsNameSeparator(char c)
{
    return c == '_' || c == '.' || c == ' ' || c == '-';
}

// builds a comparison key from a bone name: "<side>|<lowercase letters and digits>"
// side is 'l', 'r' or '-', so Foot.L, L_Foot, foot_l, LeftFoot and mixamorig:LeftFoot all become "l|foot"
static void rlmMakeBoneKey(const char* name, char* key)
{
    // drop namespace style prefixes such as "mixamorig:" or "Armature|"
    const char* start = name;
    for (const char* c = name; *c; c++)
    {
        if (*c == ':' || *c == '|')
            start = c + 1;
    }

    char lower[RLM_BONE_KEY_SIZE] = { 0 };
    int len = 0;
    for (const char* c = start; *c && len < RLM_BONE_KEY_SIZE - 1; c++)
        lower[len++] = (char)tolower((unsigned char)*c);

    char side = '-';
    char* found = NULL;
    if ((found = strstr(lower, "left")) != NULL)
    {
        side = 'l';
        memmove(found, found + 4, strlen(found + 4) + 1);
    }
    else if ((found = strstr(lower, "right")) != NULL)
    {
        side = 'r';
        memmove(found, found + 5, strlen(found + 5) + 1);
    }
    else
    {
        len = (int)strlen(lower);
        if (len >= 2 && (lower[0] == 'l' || lower[0] == 'r') && rlmIsNameSeparator(lower[1]))
        {
            side = lower[0];
            lower[0] = ' ';
        }
        else if (len >= 2 && (lower[len - 1] == 'l' || lower[len - 1] == 'r') && rlmIsNameSeparator(lower[len - 2]))
        {
            side = lower[len - 1];
            lower[len - 1] = ' ';
        }
    }

    int k = 0;
    key[k++] = side;
    key[k++] = '|';
    for (const char* c = lower; *c && k < RLM_BONE_KEY_SIZE - 1; c++)
    {
        if (isalnum((unsigned char)*c))
            key[k++] = *c;
    }
    key[k] = '\0';
}

static int rlmFindSkeletonBone(const rlmSkeleton* skeleton, const char* name)
{
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (strcmp(skeleton->bones[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int rlmMatchSourceBone(const rlmSkeleton* source, char (*sourceKeys)[RLM_BONE_KEY_SIZE], const char* targetName)
{
    int exact = rlmFindSkeletonBone(source, targetName);
    if (exact >= 0)
        return exact;

    char targetKey[RLM_BONE_KEY_SIZE];
    rlmMakeBoneKey(targetName, targetKey);
    if (targetKey[2] == '\0')
        return -1;

    int match = -1;
    for (int s = 0; s < source->boneCount; s++)
    {
        if (strcmp(sourceKeys[s], targetKey) != 0)
            continue;

        if (match >= 0)
        {
            TraceLog(LOG_DEBUG, "rlModels : Target bone %s matches more than one source bone, leaving it unmapped", targetName);
            return -1;
        }
        match = s;
    }
    return match;
}

static void rlmCountRetargetMapped(rlmRetargetMap* map)
{
    map->mappedCount = 0;
    for (int t = 0; t < map->target->boneCount; t++)
    {
        if (map->targetToSource[t] >= 0)
            map->mappedCount++;
    }
}

rlmRetargetMap rlmLoadRetargetMap(rlmSkeleton* source, rlmSkeleton* target)
{
    rlmRetargetMap map = { 0 };
    map.rootCorrection = rlmPQSIdentity();
    map.translationScale = 1;

    if (!source || !target || source->boneCount <= 0 || target->boneCount <= 0)
    {
        TraceLog(LOG_WARNING, "rlModels : Retarget map needs a source and target skeleton with bones");
        return map;
    }

    if (!source->retargetInfo)
        rlmComputeSkeletonRetargetInfo(source);
    if (!target->retargetInfo)
        rlmComputeSkeletonRetargetInfo(target);

    map.source = source;
    map.target = target;
    map.targetToSource = (int*)MemAlloc(sizeof(int) * target->boneCount);
    map.rotationOffsets = (Quaternion*)MemAlloc(sizeof(Quaternion) * target->boneCount);

    char (*sourceKeys)[RLM_BONE_KEY_SIZE] = (char (*)[RLM_BONE_KEY_SIZE])MemAlloc(RLM_BONE_KEY_SIZE * source->boneCount);
    for (int s = 0; s < source->boneCount; s++)
        rlmMakeBoneKey(source->bones[s].name, sourceKeys[s]);

    for (int t = 0; t < target->boneCount; t++)
    {
        map.targetToSource[t] = rlmMatchSourceBone(source, sourceKeys, target->bones[t].name);
        if (map.targetToSource[t] < 0)
            TraceLog(LOG_DEBUG, "rlModels : Retarget target bone %s has no source bone", target->bones[t].name);
    }
    MemFree(sourceKeys);

    rlmCountRetargetMapped(&map);
    rlmUpdateRetargetMap(&map);

    TraceLog(LOG_INFO, "rlModels : Retarget map matched %d of %d target bones by name", map.mappedCount, target->boneCount);

    int targetHip = target->retargetInfo->hipBone;
    if (targetHip >= 0 && map.targetToSource[targetHip] < 0)
        TraceLog(LOG_WARNING, "rlModels : Retarget target hip bone %s has no source bone, root motion will be lost", target->bones[targetHip].name);

    return map;
}

bool rlmSetRetargetBone(rlmRetargetMap* map, const char* targetBone, const char* sourceBone)
{
    if (!map || !map->targetToSource || !targetBone)
        return false;

    int t = rlmFindSkeletonBone(map->target, targetBone);
    if (t < 0)
    {
        TraceLog(LOG_WARNING, "rlModels : Retarget target bone %s not found", targetBone);
        return false;
    }

    int s = -1;
    if (sourceBone)
    {
        s = rlmFindSkeletonBone(map->source, sourceBone);
        if (s < 0)
        {
            TraceLog(LOG_WARNING, "rlModels : Retarget source bone %s not found", sourceBone);
            return false;
        }
    }

    map->targetToSource[t] = s;
    rlmCountRetargetMapped(map);
    rlmUpdateRetargetMap(map);
    return true;
}

void rlmSetRetargetRootCorrection(rlmRetargetMap* map, rlmPQSTransorm correction)
{
    if (!map)
        return;

    map->rootCorrection = correction;
    rlmUpdateRetargetMap(map);
}

void rlmUpdateRetargetMap(rlmRetargetMap* map)
{
    if (!map || !map->targetToSource || !map->source->retargetInfo || !map->target->retargetInfo)
        return;

    const rlmSkeletonRetargetInfo* sourceInfo = map->source->retargetInfo;
    const rlmSkeletonRetargetInfo* targetInfo = map->target->retargetInfo;
    const rlmPQSTransorm* correction = &map->rootCorrection;

    // a target bone's model space rotation is correction * sourceFrame * offset, so when the source is in its
    // reference pose the target lands exactly in its own reference pose
    Quaternion invCorrection = QuaternionInvert(correction->rotation);
    for (int t = 0; t < map->target->boneCount; t++)
    {
        int s = map->targetToSource[t];
        if (s < 0)
        {
            map->rotationOffsets[t] = QuaternionIdentity();
            continue;
        }

        Quaternion sourceRef = sourceInfo->referencePose.boneTransforms[s].rotation;
        Quaternion targetRef = targetInfo->referencePose.boneTransforms[t].rotation;
        map->rotationOffsets[t] = QuaternionNormalize(QuaternionMultiply(QuaternionMultiply(QuaternionInvert(sourceRef), invCorrection), targetRef));
    }

    map->translationScale = 1;
    if (sourceInfo->hipBone >= 0 && targetInfo->hipBone >= 0)
    {
        Vector3 sourceHip = sourceInfo->referencePose.boneTransforms[sourceInfo->hipBone].position;
        sourceHip = Vector3Add(Vector3RotateByQuaternion(Vector3Multiply(sourceHip, correction->scale), correction->rotation), correction->position);

        float sourceHeight = Vector3Length(sourceHip);
        if (sourceHeight > 0.000001f && targetInfo->hipHeight > 0.000001f)
            map->translationScale = targetInfo->hipHeight / sourceHeight;
        else
            TraceLog(LOG_WARNING, "rlModels : Retarget hip height is zero on one skeleton, hip motion will not be scaled");
    }
}

void rlmUnloadRetargetMap(rlmRetargetMap* map)
{
    if (!map)
        return;

    MemFree(map->targetToSource);
    MemFree(map->rotationOffsets);
    map->targetToSource = NULL;
    map->rotationOffsets = NULL;
    map->mappedCount = 0;
    map->source = NULL;
    map->target = NULL;
}

static Vector3 rlmTransformPoint(const rlmPQSTransorm* transform, Vector3 point)
{
    return Vector3Add(Vector3RotateByQuaternion(Vector3Multiply(point, transform->scale), transform->rotation), transform->position);
}

void rlmRetargetKeyframeInto(const rlmRetargetMap* map, const rlmAnimationKeyframe* source, rlmAnimationKeyframe* dest)
{
    if (!map || !map->targetToSource || !map->source || !map->target || !map->source->retargetInfo || !map->target->retargetInfo)
        return;

    if (!source || !dest || !source->boneTransforms || !dest->boneTransforms || source->boneTransforms == dest->boneTransforms)
        return;

    const rlmSkeleton* target = map->target;
    const rlmSkeletonRetargetInfo* sourceInfo = map->source->retargetInfo;
    const rlmSkeletonRetargetInfo* targetInfo = target->retargetInfo;
    const rlmPQSTransorm* correction = &map->rootCorrection;

    // parents first, so every bone can build on its parent's finished model space transform in dest
    for (int i = 0; i < target->boneCount; i++)
    {
        int t = targetInfo->evalOrder[i];
        int parent = target->bones[t].parentId;
        rlmPQSTransorm local = targetInfo->localReferencePose.boneTransforms[t];

        // start from the target's own reference pose hanging off the (already retargeted) parent,
        // this keeps the target's bone lengths and is what unmapped bones end up with
        rlmPQSTransorm result = local;
        if (parent >= 0)
        {
            const rlmPQSTransorm* parentModel = &dest->boneTransforms[parent];
            result.rotation = QuaternionMultiply(parentModel->rotation, local.rotation);
            result.scale = Vector3Multiply(local.scale, parentModel->scale);
            result.position = rlmTransformPoint(parentModel, local.position);
        }

        int s = map->targetToSource[t];
        if (s >= 0)
        {
            const rlmPQSTransorm* sourceFrame = &source->boneTransforms[s];
            result.rotation = QuaternionNormalize(QuaternionMultiply(QuaternionMultiply(correction->rotation, sourceFrame->rotation), map->rotationOffsets[t]));

            if (t == targetInfo->hipBone)
            {
                // hip motion relative to the source reference pose, scaled to the target's size
                Vector3 current = rlmTransformPoint(correction, sourceFrame->position);
                Vector3 reference = rlmTransformPoint(correction, sourceInfo->referencePose.boneTransforms[s].position);
                Vector3 motion = Vector3Scale(Vector3Subtract(current, reference), map->translationScale);

                result.position = Vector3Add(targetInfo->referencePose.boneTransforms[t].position, motion);
            }
        }

        dest->boneTransforms[t] = result;
    }
}

rlmAnimationKeyframe rlmRetargetKeyframe(const rlmRetargetMap* map, rlmAnimationKeyframe source)
{
    rlmAnimationKeyframe frame = { 0 };
    if (!map || !map->target || map->target->boneCount <= 0)
        return frame;

    frame.boneTransforms = (rlmPQSTransorm*)MemAlloc(sizeof(rlmPQSTransorm) * map->target->boneCount);
    rlmRetargetKeyframeInto(map, &source, &frame);
    return frame;
}

rlmModelAniamtionSequence rlmRetargetSequence(const rlmRetargetMap* map, const rlmModelAniamtionSequence* source)
{
    rlmModelAniamtionSequence sequence = { 0 };
    if (!map || !map->target || !source)
        return sequence;

    memcpy(sequence.name, source->name, sizeof(sequence.name));
    sequence.fps = source->fps;

    if (source->keyframeCount <= 0 || !source->keyframes)
        return sequence;

    sequence.keyframeCount = source->keyframeCount;
    sequence.keyframes = (rlmAnimationKeyframe*)MemAlloc(sizeof(rlmAnimationKeyframe) * sequence.keyframeCount);

    for (int f = 0; f < sequence.keyframeCount; f++)
        sequence.keyframes[f] = rlmRetargetKeyframe(map, source->keyframes[f]);

    return sequence;
}

rlmModelAnimationSet rlmRetargetAnimationSet(const rlmRetargetMap* map, const rlmModelAnimationSet* source)
{
    rlmModelAnimationSet set = { 0 };
    if (!map || !source || source->sequenceCount <= 0 || !source->sequences)
        return set;

    set.sequenceCount = source->sequenceCount;
    set.sequences = (rlmModelAniamtionSequence*)MemAlloc(sizeof(rlmModelAniamtionSequence) * set.sequenceCount);

    for (int i = 0; i < set.sequenceCount; i++)
        set.sequences[i] = rlmRetargetSequence(map, &source->sequences[i]);

    return set;
}

static void rlmModelToLocalRecursive(const rlmBoneInfo* bone, const rlmAnimationKeyframe* in, rlmAnimationKeyframe* out, const rlmPQSTransorm* parentModel)
{
    // children first, so that when in == out this bone's model space transform is still intact while the children use it
    rlmPQSTransorm thisModel = in->boneTransforms[bone->boneId];

    for (int i = 0; i < bone->childCount; i++)
        rlmModelToLocalRecursive(bone->childBones[i], in, out, &thisModel);

    if (!parentModel)
    {
        out->boneTransforms[bone->boneId] = thisModel;
        return;
    }

    Quaternion invParentRotation = QuaternionInvert(parentModel->rotation);
    Vector3 invParentScale = Vector3Divide((Vector3) { 1, 1, 1 }, parentModel->scale);

    rlmPQSTransorm local;
    local.rotation = QuaternionMultiply(invParentRotation, thisModel.rotation);
    local.scale = Vector3Multiply(thisModel.scale, invParentScale);
    local.position = Vector3Multiply(Vector3RotateByQuaternion(Vector3Subtract(thisModel.position, parentModel->position), invParentRotation), invParentScale);

    out->boneTransforms[bone->boneId] = local;
}

static void rlmLocalToModelRecursive(const rlmBoneInfo* bone, const rlmAnimationKeyframe* in, rlmAnimationKeyframe* out, const rlmPQSTransorm* parentModel)
{
    // parents first, children read their parent's finished model space transform from out
    rlmPQSTransorm local = in->boneTransforms[bone->boneId];
    rlmPQSTransorm thisModel = local;

    if (parentModel)
    {
        thisModel.rotation = QuaternionMultiply(parentModel->rotation, local.rotation);
        thisModel.scale = Vector3Multiply(local.scale, parentModel->scale);
        thisModel.position = Vector3Add(Vector3RotateByQuaternion(Vector3Multiply(local.position, parentModel->scale), parentModel->rotation), parentModel->position);
    }

    out->boneTransforms[bone->boneId] = thisModel;

    for (int i = 0; i < bone->childCount; i++)
        rlmLocalToModelRecursive(bone->childBones[i], in, out, &thisModel);
}

void rlmKeyframeModelToLocal(const rlmSkeleton* skeleton, const rlmAnimationKeyframe* in, rlmAnimationKeyframe* out)
{
    if (!skeleton || !in || !out || !in->boneTransforms || !out->boneTransforms)
        return;

    // walk every root, not just rootBone, so skeletons with multiple roots are fully converted
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (skeleton->bones[i].parentId < 0)
            rlmModelToLocalRecursive(&skeleton->bones[i], in, out, NULL);
    }
}

void rlmKeyframeLocalToModel(const rlmSkeleton* skeleton, const rlmAnimationKeyframe* in, rlmAnimationKeyframe* out)
{
    if (!skeleton || !in || !out || !in->boneTransforms || !out->boneTransforms)
        return;

    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (skeleton->bones[i].parentId < 0)
            rlmLocalToModelRecursive(&skeleton->bones[i], in, out, NULL);
    }
}

void rlmAdvanceAnimationInstance(rlmAnimatedModelInstance* instance, float deltaTime)
{
    if (!instance || !instance->sequences || instance->currentSequence < 0 || instance->currentSequence >= instance->sequences->sequenceCount)
        return;

    if (instance->sequences->sequences[instance->currentSequence].keyframeCount <= 0)
        return;

    instance->currentParam += deltaTime;

    float fpsDelta = 1.0f / instance->sequences->sequences[instance->currentSequence].fps;

    while (instance->currentParam >= fpsDelta)
    {
        instance->currentParam -= fpsDelta;
        instance->currentFrame++;
        if (instance->currentFrame >= instance->sequences->sequences[instance->currentSequence].keyframeCount)
            instance->currentFrame = 0;

        if (!instance->interpolate)
            rlmSetPoseToKeyframe(*instance->model, &instance->currentPose, instance->sequences->sequences[instance->currentSequence].keyframes[instance->currentFrame]);
    }

    if (instance->interpolate)
    {
        int nextFrame = instance->currentFrame + 1;
        if (nextFrame >= instance->sequences->sequences[instance->currentSequence].keyframeCount)
            nextFrame = 0;

        rlmSetPoseToKeyframesLerp(*instance->model,
            &instance->currentPose,
            instance->sequences->sequences[instance->currentSequence].keyframes[instance->currentFrame],
            instance->sequences->sequences[instance->currentSequence].keyframes[nextFrame],
            instance->currentParam);
    }
}

void rlmSetAnimationInstanceSequence(rlmAnimatedModelInstance* instance, int sequence)
{
    if (!instance || !instance->sequences || sequence < 0 || sequence >= instance->sequences->sequenceCount)
        return;
    instance->currentFrame = 0;
    instance->currentSequence = sequence;
    instance->currentParam = -1.0f / instance->sequences->sequences[instance->currentSequence].fps;

    rlmAdvanceAnimationInstance(instance, 0);
}

void rlmUnloadAnimationPose(rlmModelAnimationPose* pose)
{
    MemFree(pose->boneMatricies);
    pose->boneMatricies = NULL;
}

void rlmUnloadAnimationKeyframe(rlmAnimationKeyframe* keyframe)
{
    MemFree(keyframe->boneTransforms);
    keyframe->boneTransforms = NULL;
}

void rlmUnloadAnimationSequence(rlmModelAniamtionSequence* sequence)
{
    if (!sequence)
        return;

    for (int i = 0; i < sequence->keyframeCount; i++)
        rlmUnloadAnimationKeyframe(sequence->keyframes + i);

    MemFree(sequence->keyframes);
    sequence->keyframeCount = 0;
    sequence->keyframes = NULL;
}

void rlmUnloadAnimationSet(rlmModelAnimationSet* set)
{
    if (!set)
        return;

    for (int i = 0; i < set->sequenceCount; i++)
        rlmUnloadAnimationSequence(set->sequences + i);

    MemFree(set->sequences);
    set->sequenceCount = 0;
    set->sequences = NULL;
}

rlmPQSTransorm rlmPQSIdentity()
{
    rlmPQSTransorm transform;
    transform.position = Vector3Zero();
    transform.rotation = QuaternionIdentity();
    transform.scale = (Vector3){ 1,1,1 };

    return transform;
}

rlmPQSTransorm rlmPQSTranslation(float x, float y, float z)
{
    rlmPQSTransorm transform;
    transform.position = (Vector3){ x,y,z };
    transform.rotation = QuaternionIdentity();
    transform.scale = (Vector3){ 1,1,1 };

    return transform;
}

rlmPQSTransorm rlmPQSTransformAdd(rlmPQSTransorm lhs, rlmPQSTransorm rhs)
{
    return rlmPQSFromMatrix(MatrixAdd(rlmPQSToMatrix(&lhs), rlmPQSToMatrix(&rhs)));
}

rlmPQSTransorm rlmPQSTransformSubtract(rlmPQSTransorm lhs, rlmPQSTransorm rhs)
{
    return rlmPQSFromMatrix(MatrixSubtract(rlmPQSToMatrix(&lhs), rlmPQSToMatrix(&rhs)));
}

rlmPQSTransorm rlmPQSLerp(const rlmPQSTransorm* lhs, const rlmPQSTransorm* rhs, float param)
{
    rlmPQSTransorm transform;
    transform.position = Vector3Lerp(lhs->position, rhs->position, param);
    transform.rotation = QuaternionSlerp(lhs->rotation, rhs->rotation, param);
    transform.scale = Vector3Lerp(lhs->scale, rhs->scale, param);

    return transform;
}

RayCollision rlmGetRayCollisionMesh(Ray ray, const rlmMesh* mesh, rlmPQSTransorm transform)
{
    RayCollision collision = { 0 };

    // Check if mesh vertex data on CPU for testing
    if (mesh == NULL || mesh->meshBuffers == NULL)
        return collision;

    collision.hit = false;

    Matrix transformMat = rlmPQSToMatrix(&transform);
    transformMat = MatrixMultiply(rlmPQSToMatrix(&mesh->transform), transformMat);

    Quaternion transformRot = QuaternionFromMatrix(transformMat);

    Matrix invMatrix = MatrixInvert(transformMat);

    // transform the ray into the model, so we don't have to transform a bazillion verts
    Vector3 orign = ray.position;

    ray.position = Vector3Transform(ray.position, invMatrix);
    Quaternion rot = QuaternionFromMatrix(invMatrix);
    ray.direction = Vector3RotateByQuaternion(ray.direction, rot);

    if (!GetRayCollisionBox(ray, mesh->bounds).hit)
    {
        return collision;
    }

    int triangleCount = mesh->meshBuffers->triangleCount;

    // Test against all triangles in mesh
    for (int i = 0; i < triangleCount; i++)
    {
        Vector3 a = { 0 };
        Vector3 b = { 0 };
        Vector3 c = { 0 };
        Vector3* vertdata = (Vector3*)mesh->meshBuffers->vertices;

        if (mesh->meshBuffers->indices)
        {
            a = vertdata[mesh->meshBuffers->indices[i * 3 + 0]];
            b = vertdata[mesh->meshBuffers->indices[i * 3 + 1]];
            c = vertdata[mesh->meshBuffers->indices[i * 3 + 2]];
        }
        else
        {
            a = vertdata[i * 3 + 0];
            b = vertdata[i * 3 + 1];
            c = vertdata[i * 3 + 2];
        }

        RayCollision triHitInfo = GetRayCollisionTriangle(ray, a, b, c);

        if (triHitInfo.hit)
        {
            triHitInfo.point = Vector3Transform(triHitInfo.point, transformMat);
            triHitInfo.normal = Vector3RotateByQuaternion(triHitInfo.normal, transformRot);
            // Save the closest hit triangle
            if ((!collision.hit) || (collision.distance > triHitInfo.distance)) collision = triHitInfo;
        }
    }

    return collision;
}

RayCollision rlmGetRayCollisionModel(Ray ray, const rlmModel* model, rlmPQSTransorm transform, rlmMesh** hitMesh)
{
    RayCollision collision = { 0 };

    if (hitMesh)
    {
        *hitMesh = NULL;
    }

    if (model == NULL)
        return collision;


    // multiply in the model matrix
    Matrix mat = rlmPQSToMatrix(&transform);
    Matrix modelMat = rlmPQSToMatrix(&model->orientationTransform);
    mat = MatrixMultiply(modelMat, mat);

    transform = rlmPQSFromMatrix(mat);

    for (int group = 0; group < model->groupCount; group++)
    {
        rlmModelGroup* groupPtr = model->groups + group;

        for (int meshIndex = 0; meshIndex < groupPtr->meshCount; meshIndex++)
        {
            rlmMesh* mesh = groupPtr->meshes + meshIndex;

            RayCollision meshCollision = rlmGetRayCollisionMesh(ray, mesh, transform);
            if (meshCollision.hit)
            {
                if (!collision.hit || meshCollision.distance < collision.distance)
                {
                    collision = meshCollision;
                    if (hitMesh)
                    {
                        *hitMesh = mesh;
                    }
                }
            }
        }
    }
    return collision;
}