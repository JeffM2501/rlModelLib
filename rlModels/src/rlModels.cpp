#include "rlModels.h"

#include "rlgl.h"
#include "config.h"

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

#ifdef RL_SUPPORT_MESH_GPU_SKINNING
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS] = 0;      // Vertex buffer: boneIds
    mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS] = 0;  // Vertex buffer: boneWeights
#endif

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

#ifdef RL_SUPPORT_MESH_GPU_SKINNING
    if (mesh->meshBuffers->boneIds != NULL)
    {
        // Enable vertex attribute: boneIds (shader-location = 7)
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS] = rlLoadVertexBuffer(mesh->meshBuffers->boneIds, mesh->meshBuffers->vertexCount * 4 * sizeof(unsigned char), dynamic);
        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS, 4, RL_UNSIGNED_BYTE, 0, 0, 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS);
    }
    else
    {
        // Default vertex attribute: boneIds
        // WARNING: Default value provided to shader if location available
        float value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS, value, SHADER_ATTRIB_VEC4, 4);
        rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS);
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
#endif

    if (mesh->meshBuffers->indices != NULL)
    {
        mesh->gpuMesh.vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES] = rlLoadVertexBufferElement(mesh->meshBuffers->indices, mesh->meshBuffers->triangleCount * 3 * sizeof(unsigned short), dynamic);
    }

    if (mesh->gpuMesh.vaoId > 0) TRACELOG(LOG_INFO, "VAO: [ID %i] Mesh uploaded successfully to VRAM (GPU)", mesh->vaoId);
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
    material.shader.id = rlGetShaderIdDefault();
    material.shader.locs = rlGetShaderLocsDefault();
    material.ownsShader = true;

    material.baseChannel.ownsTexture = false;
    material.baseChannel.textureId = rlGetTextureIdDefault();
    material.baseChannel.shaderLoc = -1;

    material.extraChannels = 0;
    material.extraChannels = NULL;

    return material;
}

void rlmUnloadMaterial(rlmMaterialDef* material)
{
    if (!material)
        return;

    if (material->ownsShader)
    {
        if (material->shader.id != rlGetShaderIdDefault())
        {
            rlUnloadShaderProgram(material->shader.id);

            // NOTE: If shader loading failed, it should be 0
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
}

void rlmAddMaterialChannel(rlmMaterialDef* material, Texture2D texture, int shaderLoc, bool isCubeMap)
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
    material->extraChannels[material->materialChannels - 1].shaderLoc = shaderLoc;
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

        if (textures && locs)
        {
            material->extraChannels[material->materialChannels - 1].textureId = textures[i].id;
            material->extraChannels[material->materialChannels - 1].shaderLoc = locs[i];
        }
        else
        {
            material->extraChannels[material->materialChannels - 1].textureId = 0;
            material->extraChannels[material->materialChannels - 1].shaderLoc = -1;
        }
    }
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

    newMaterial->baseChannel = oldMaterial->baseChannel;
    newMaterial->baseChannel.ownsTexture = false;

    newMaterial->shader = oldMaterial->shader;
    newMaterial->ownsShader = false;

    newMaterial->tint = oldMaterial->tint;

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
}

rlmModel rlmCloneModel(rlmModel model)
{
    rlmModel newModel;
    newModel.groupCount = model.groupCount;

    newModel.groups = (rlmModelGroup*)MemAlloc(sizeof(rlmModelGroup) * newModel.groupCount);
    for (int group = 0; group < newModel.groupCount; group++)
    {
        rlmModelGroup* newGroup = newModel.groups + group;
        const rlmModelGroup* oldGroup = model.groups + group;

        rlmCloneMaterial(&oldGroup->material, &newGroup->material);

        newGroup->ownsMeshes = false;
        newGroup->meshCount = oldGroup->meshCount;

        newGroup->meshes = (rlmMesh*)MemAlloc(sizeof(rlmMesh) * newGroup->meshCount);

        for (int m = 0; m < newGroup->meshCount; m++)
        {
            newGroup->meshes[m] = oldGroup->meshes[m];
        }
    }

    return newModel;
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

        MemFree(groupPtr->meshes);
        groupPtr->meshCount = 0;
        groupPtr->meshes = 0;
    }

    model->groupCount = 0;
    MemFree(model->groups);
    model->groups = NULL;
}

void rlmApplyMaterialDef(rlmMaterialDef* material)
{
    if (!material)
        return;

    rlEnableShader(material->shader.id);

    int index = 0;

    rlActiveTextureSlot(0);
    rlEnableTexture(material->baseChannel.textureId);
    rlSetUniform(material->baseChannel.shaderLoc, &index, SHADER_UNIFORM_INT, 1);

    for (index = 1; index < material->materialChannels+1; index++)
    {
        rlActiveTextureSlot(index);

        if (material->extraChannels[index - 1].cubeMap)
            rlEnableTextureCubemap(material->extraChannels[index - 1].textureId);
        else
            rlEnableTexture(material->extraChannels[index-1].textureId);
        
        rlSetUniform(material->extraChannels[index - 1].shaderLoc, &index, SHADER_UNIFORM_INT, 1);
    }
}

Matrix rlmPQSToMatrix(const rlmPQSTransorm* transform)
{
    Matrix matScale = MatrixScale(transform->scale.x, transform->scale.y, transform->scale.z);

    Vector3 axis = { 0 };
    float angle = 0;

    QuaternionToAxisAngle(transform->rotation, &axis, &angle);
    Matrix matRotation = MatrixRotate(axis, angle);
    Matrix matTranslation = MatrixTranslate(transform->translation.x, transform->translation.y, transform->translation.z);

    return MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
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

#ifdef RL_SUPPORT_MESH_GPU_SKINNING
        // Bind mesh VBO data: vertex bone ids (shader-location = 6, if available)
        if (shader->locs[SHADER_LOC_VERTEX_BONEIDS] != -1)
        {
            rlEnableVertexBuffer(mesh->vboIds[RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEIDS]);
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
#endif

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

        float values[4] = {
           (float)groupPtr->material.tint.r / 255.0f,
           (float)groupPtr->material.tint.g / 255.0f,
           (float)groupPtr->material.tint.b / 255.0f,
           (float)groupPtr->material.tint.a / 255.0f
        };
        rlSetUniform(groupPtr->material.shader.locs[SHADER_LOC_COLOR_DIFFUSE], values, SHADER_UNIFORM_VEC4, 1);

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
        
        // draw the meshes
        for (int i = 0; i < groupPtr->meshCount; i++)
            rlmDrawMesh(&groupPtr->meshes[i].gpuMesh, &groupPtr->material.shader);

        // Disable shader program
        rlDisableShader();
    }
}

rlmPQSTransorm rlmPQSIdentity()
{
    rlmPQSTransorm transform;
    transform.translation = Vector3Zero();
    transform.rotation = QuaternionIdentity();
    transform.scale = Vector3{ 1,1,1 };

    return transform;
}