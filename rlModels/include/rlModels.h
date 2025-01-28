
#pragma once

#include "raylib.h"
#include "raymath.h"

typedef struct rlmGPUMesh	// data needed to draw a mesh
{
    unsigned int vaoId;     // OpenGL Vertex Array Object id
    unsigned int* vboIds;    // OpenGL Vertex Buffer Objects id (default vertex data)

    bool isIndexed;
    unsigned int elementCount;

}rlmGPUMesh;

typedef struct rlmMeshBuffers	// geometry buffers used to define a mesh
{
    int vertexCount;        // Number of vertices stored in arrays
    int triangleCount;      // Number of triangles stored (indexed or not)

    // Vertex attributes data
    float* vertices;        // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float* texcoords;       // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float* texcoords2;      // Vertex texture second coordinates (UV - 2 components per vertex) (shader-location = 5)
    float* normals;         // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
    float* tangents;        // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
    unsigned char* colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    unsigned short* indices;    // Vertex indices (in case vertex data comes indexed)

    // Animation vertex data
    unsigned char* boneIds; // Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning) (shader-location = 6)
    float* boneWeights;     // Vertex bone weight, up to 4 bones influence by vertex (skinning) (shader-location = 7)
}rlmMeshBuffers;

typedef struct rlmMesh // a mesh
{
    char* name;
    rlmGPUMesh gpuMesh;
    rlmMeshBuffers* meshBuffers; // optional after upload
    // TODO rlmAnimatedMeshBuffers ?
}rlmMesh;

typedef struct rlmMaterialChannel // a texture map in a material
{
    int textureId;
    bool ownsTexture;
    int shaderLoc;
    bool cubeMap;
    int textureSlot;
}rlmMaterialChannel;

typedef struct rlmMaterialDef // a shader and it's input texture
{
    char* name; 
    Shader shader;
    bool ownsShader;

    rlmMaterialChannel	baseChannel;
    Color tint;		                // TODO, push this into the channel?

    int materialChannels;
    rlmMaterialChannel* extraChannels;
}rlmMaterialDef; 

typedef struct rlmModelGroup // a group of meshes that share a material
{
    rlmMaterialDef material;
    bool ownsMeshes;
    bool ownsMeshList;
    int meshCount;
    rlmMesh* meshes;
}rlmModelGroup; 

typedef struct rlmPQSTransorm // a transform
{
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
}rlmPQSTransorm;

typedef struct rlmModel // a group of meshes, materials, and an orientation transform
{
    int groupCount;
    rlmModelGroup* groups;
    rlmPQSTransorm orientationTransform;
}rlmModel;

// meshes
void rlmUploadMesh(rlmMesh* mesh, bool releaseGeoBuffers);

// materials
rlmMaterialDef rlmGetDefaultMaterial();

void rlmUnloadMaterial(rlmMaterialDef* material);

void rlmAddMaterialChannel(rlmMaterialDef* material, Texture2D texture, int shaderLoc, bool isCubeMap);
void rlmAddMaterialChannels(rlmMaterialDef* material, int count, Texture2D* textures, int* locs);

void rlmSetMaterialDefShader(rlmMaterialDef* material, Shader shader);

void rlmSetMaterialChannelTexture(rlmMaterialChannel* channel, Texture2D texture);

// models
void rlmSetModelShader(rlmModel* model, Shader shader);

rlmModel rlmCloneModel(rlmModel model);

void rlmUnloadModel(rlmModel* model);

void rlmApplyMaterialDef(rlmMaterialDef* material);

void rlmDrawModel(rlmModel model, rlmPQSTransorm transform);

rlmPQSTransorm rlmPQSIdentity();

// state API
void rlmSetDefaultMaterialShader(Shader shader);
void rlmClearDefaultMaterialShader();
