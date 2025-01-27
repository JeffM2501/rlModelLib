
#pragma once

#include "raylib.h"
#include "raymath.h"

typedef struct rlmGPUMesh	// data needed to draw a mesh
{
    unsigned int vaoId;     // OpenGL Vertex Array Object id
    unsigned int* vboIds;    // OpenGL Vertex Buffer Objects id (default vertex data)

    bool isIndexed;
    unsigned int elementCount;

}rlmGPUMesh; // 12 bytes

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
}rlmMeshBuffers;// 9*size_t bytes

typedef struct rlmMesh // a mesh
{
    rlmGPUMesh gpuMesh;
    rlmMeshBuffers* meshBuffers; // optional after upload
}rlmMesh; //20 bytes

typedef struct rlmMaterialChannel // a texture map in a material
{
    int textureId;	//4
    bool ownsTexture; //1
    int shaderLoc;	//4
    bool cubeMap;
    int textureSlot;
}rlmMaterialChannel; // 9 bytes

typedef struct rlmMaterialDef // a shader and it's input texture
{
    Shader shader;	// 12
    bool ownsShader; //1

    rlmMaterialChannel	baseChannel; //9
    Color tint;		//4              // TODO, push this into the channel?

    int materialChannels;			//4
    rlmMaterialChannel* extraChannels;// 8
}rlmMaterialDef; // 38 bytes

typedef struct rlmModelGroup // a group of meshes that share a material
{
    rlmMaterialDef material;
    bool ownsMeshes;
    int meshCount;
    rlmMesh* meshes;
}rlmModelGroup; //50 bytes

typedef struct rlmPQSTransorm // a transform
{
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
}rlmPQSTransorm;// 40 bytes

typedef struct rlmModel // a group of meshes, materials, and an orientation transform
{
    int groupCount;	// 4
    rlmModelGroup* groups; // 8
    rlmPQSTransorm orientationTransform; // 40 (store as pointer? )
}rlmModel; // 53 bytes

void rlmUploadMesh(rlmMesh* mesh, bool releaseGeoBuffers);
void rlmUnloadMesh(rlmMesh* mesh);

rlmMaterialDef rlmGetDefaultMaterial();

void rlmUnloadMaterial(rlmMaterialDef* material);

void rlmAddMaterialChannel(rlmMaterialDef* material, Texture2D texture, int shaderLoc, bool isCubeMap);
void rlmAddMaterialChannels(rlmMaterialDef* material, int count, Texture2D* textures, int* locs);

void rlmSetMaterialDefShader(rlmMaterialDef* material, Shader shader);

void rlmSetMaterialChannelTexture(rlmMaterialChannel* channel, Texture2D texture);

rlmModel rlmCloneModel(rlmModel model);

void rlmUnloadModel(rlmModel* model);

void rlmApplyMaterialDef(rlmMaterialDef* material);

void rlmDrawModel(rlmModel model, rlmPQSTransorm transform);

rlmPQSTransorm rlmPQSIdentity();
