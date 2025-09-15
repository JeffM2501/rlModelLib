#include "external/cgltf.h"
#include "raylib.h"
#include "rlModels.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include <string.h>
#include <stdlib.h>

#define LOAD_ATTRIBUTE(accesor, numComp, srcType, dstPtr) LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, srcType)

#define LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, dstType) \
    { \
        int n = 0; \
        srcType *buffer = (srcType *)accesor->buffer_view->buffer->data + accesor->buffer_view->offset/sizeof(srcType) + accesor->offset/sizeof(srcType); \
        for (unsigned int k = 0; k < accesor->count; k++) \
        {\
            for (int l = 0; l < numComp; l++) \
            {\
                dstPtr[numComp*k + l] = (dstType)buffer[n + l];\
            }\
            n += (int)(accesor->stride/sizeof(srcType));\
        }\
    }

// Load file data callback for cgltf
static cgltf_result LoadFileGLTFCallback(const struct cgltf_memory_options* memoryOptions, const struct cgltf_file_options* fileOptions, const char* path, cgltf_size* size, void** data)
{
    int filesize;
    unsigned char* filedata = LoadFileData(path, &filesize);

    if (filedata == NULL) return cgltf_result_io_error;

    *size = filesize;
    *data = filedata;

    return cgltf_result_success;
}

// Release file data callback for cgltf
static void ReleaseFileGLTFCallback(const struct cgltf_memory_options* memoryOptions, const struct cgltf_file_options* fileOptions, void* data)
{
    UnloadFileData((unsigned char*)data);
}

int GetGLTFPrimCount(cgltf_data* data)
{
    int primitivesCount = 0;
    // NOTE: We will load every primitive in the glTF as a separate raylib Mesh.
    // Determine total number of meshes needed from the node hierarchy.
    for (unsigned int i = 0; i < data->nodes_count; i++)
    {
        cgltf_node* node = &(data->nodes[i]);
        cgltf_mesh* mesh = node->mesh;
        if (!mesh)
            continue;

        for (unsigned int p = 0; p < mesh->primitives_count; p++)
        {
            if (mesh->primitives[p].type == cgltf_primitive_type_triangles)
                primitivesCount++;
        }
    }

    return primitivesCount;
}

static Image LoadImageFromCgltfImage(cgltf_image* cgltfImage, const char* texPath)
{
    Image image = { 0 };

    if (cgltfImage == NULL) 
        return image;

    if (cgltfImage->uri != NULL)     // Check if image data is provided as an uri (base64 or path)
    {
        if ((strlen(cgltfImage->uri) > 5) &&
            (cgltfImage->uri[0] == 'd') &&
            (cgltfImage->uri[1] == 'a') &&
            (cgltfImage->uri[2] == 't') &&
            (cgltfImage->uri[3] == 'a') &&
            (cgltfImage->uri[4] == ':'))     // Check if image is provided as base64 text data
        {
            // Data URI Format: data:<mediatype>;base64,<data>

            // Find the comma
            int i = 0;
            while ((cgltfImage->uri[i] != ',') && (cgltfImage->uri[i] != 0))
                i++;

            if (cgltfImage->uri[i] != 0)
            {
                int base64Size = (int)strlen(cgltfImage->uri + i + 1);
                while (cgltfImage->uri[i + base64Size] == '=') base64Size--;    // Ignore optional paddings
                int numberOfEncodedBits = base64Size * 6 - (base64Size * 6) % 8;   // Encoded bits minus extra bits, so it becomes a multiple of 8 bits
                int outSize = numberOfEncodedBits / 8;                           // Actual encoded bytes
                void* data = NULL;

                cgltf_options options = {};
                options.file.read = LoadFileGLTFCallback;
                options.file.release = ReleaseFileGLTFCallback;
                cgltf_result result = cgltf_load_buffer_base64(&options, outSize, cgltfImage->uri + i + 1, &data);

                if (result == cgltf_result_success)
                {
                    image = LoadImageFromMemory(".png", (unsigned char*)data, outSize);
                    MemFree(data);
                }
            }
        }
        else     // Check if image is provided as image path
        {
            image = LoadImage(TextFormat("%s/%s", texPath, cgltfImage->uri));
        }
    }
    else if ((cgltfImage->buffer_view != NULL) && (cgltfImage->buffer_view->buffer->data != NULL))    // Check if image is provided as data buffer
    {
        unsigned char* data = (unsigned char*)MemAlloc((unsigned int)(cgltfImage->buffer_view->size));
        int offset = (int)cgltfImage->buffer_view->offset;
        int stride = (int)cgltfImage->buffer_view->stride ? (int)cgltfImage->buffer_view->stride : 1;

        // Copy buffer data to memory for loading
        for (unsigned int i = 0; i < cgltfImage->buffer_view->size; i++)
        {
            data[i] = ((unsigned char*)cgltfImage->buffer_view->buffer->data)[offset];
            offset += stride;
        }

        // Check mime_type for image: (cgltfImage->mime_type == "image/png")
        // NOTE: Detected that some models define mime_type as "image\\/png"
        if ((strcmp(cgltfImage->mime_type, "image\\/png") == 0) || (strcmp(cgltfImage->mime_type, "image/png") == 0))
        {
            image = LoadImageFromMemory(".png", data, (int)cgltfImage->buffer_view->size);
        }
        else if ((strcmp(cgltfImage->mime_type, "image\\/jpeg") == 0) || (strcmp(cgltfImage->mime_type, "image/jpeg") == 0))
        {
            image = LoadImageFromMemory(".jpg", data, (int)cgltfImage->buffer_view->size);
        }

        MemFree(data);
    }

    return image;
}

void LoadPBRMaterial(cgltf_material* mat, rlmMaterialDef* material)
{
    material->shader.id = rlGetShaderIdDefault();
    material->shader.locs = rlGetShaderLocsDefault();

    material->baseChannel.ownsTexture = false;
    material->baseChannel.textureId = rlGetTextureIdDefault();
    material->baseChannel.textureLoc = -1;

    material->baseChannel.color = WHITE;
    material->baseChannel.colorLoc = -SHADER_LOC_COLOR_DIFFUSE;

    // Load base color texture (albedo)
    if (mat->pbr_metallic_roughness.base_color_texture.texture)
    {
        Image imAlbedo = LoadImageFromCgltfImage(mat->pbr_metallic_roughness.base_color_texture.texture->image, "");
        if (imAlbedo.data != NULL)
        {
            material->baseChannel.ownsTexture = true;
            material->baseChannel.textureId = LoadTextureFromImage(imAlbedo).id;
            UnloadImage(imAlbedo);
        }
    }

    // Load base color factor (tint)
    material->baseChannel.color.r = (unsigned char)(mat->pbr_metallic_roughness.base_color_factor[0] * 255);
    material->baseChannel.color.g = (unsigned char)(mat->pbr_metallic_roughness.base_color_factor[1] * 255);
    material->baseChannel.color.b = (unsigned char)(mat->pbr_metallic_roughness.base_color_factor[2] * 255);
    material->baseChannel.color.a = (unsigned char)(mat->pbr_metallic_roughness.base_color_factor[3] * 255);

    // Load metallic/roughness texture
    if (mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
    {
        Image imMetallicRoughness = LoadImageFromCgltfImage(mat->pbr_metallic_roughness.metallic_roughness_texture.texture->image, "");
        if (imMetallicRoughness.data != NULL)
        {
            Image imMetallic = { 0 };
            Image imRoughness = { 0 };

            imMetallic.data = RL_MALLOC(imMetallicRoughness.width * imMetallicRoughness.height);
            imRoughness.data = RL_MALLOC(imMetallicRoughness.width * imMetallicRoughness.height);

            imMetallic.width = imRoughness.width = imMetallicRoughness.width;
            imMetallic.height = imRoughness.height = imMetallicRoughness.height;

            imMetallic.format = imRoughness.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
            imMetallic.mipmaps = imRoughness.mipmaps = 1;

            for (int x = 0; x < imRoughness.width; x++)
            {
                for (int y = 0; y < imRoughness.height; y++)
                {
                    Color color = GetImageColor(imMetallicRoughness, x, y);

                    ((unsigned char*)imRoughness.data)[y * imRoughness.width + x] = color.g; // Roughness color channel
                    ((unsigned char*)imMetallic.data)[y * imMetallic.width + x] = color.b; // Metallic color channel
                }
            }

            unsigned char metalnessValue = (unsigned char)(mat->pbr_metallic_roughness.metallic_factor * 255);
            material->extraChannels[0].color = (Color){ metalnessValue,metalnessValue,metalnessValue,255 };

            material->extraChannels[0].textureSlot = MATERIAL_MAP_METALNESS;
            material->extraChannels[0].textureLoc = material->shader.locs[SHADER_LOC_MAP_METALNESS];

            material->extraChannels[0].ownsTexture = true;
            material->extraChannels[0].textureId = LoadTextureFromImage(imMetallic).id;


            unsigned char roughnessValue = (unsigned char)(mat->pbr_metallic_roughness.roughness_factor * 255);
            material->extraChannels[1].color = (Color){ roughnessValue,roughnessValue,roughnessValue,255 };

            material->extraChannels[1].textureSlot = MATERIAL_MAP_ROUGHNESS;
            material->extraChannels[1].textureLoc = material->shader.locs[SHADER_LOC_MAP_ROUGHNESS];

            material->extraChannels[1].ownsTexture = true;
            material->extraChannels[1].textureId = LoadTextureFromImage(imRoughness).id;

            UnloadImage(imRoughness);
            UnloadImage(imMetallic);
            UnloadImage(imMetallicRoughness);
        }
    }
}

void LoadMaterial(cgltf_material* mat, rlmMaterialDef* material)
{
    int extraChannelCount = 0;
    if (mat->name)
    {
        material->name = RL_MALLOC(strlen(mat->name) + 1);
        strcpy(material->name, mat->name);
    }
    else
    {
        material->name = (char*)MemAlloc(32);
        material->name[0] = '\0';
        strcpy(material->name, "Unknown Material");
    }

    if (mat->emissive_texture.texture)
        extraChannelCount++;
    if (mat->occlusion_texture.texture)
        extraChannelCount++;
    if (mat->normal_texture.texture)
        extraChannelCount++;

    int nextChannel = 0;

    if (mat->has_pbr_metallic_roughness)
    {
        extraChannelCount += 2;
   
    }
    else if (mat->has_pbr_specular_glossiness)
    {

    }

    material->extraChannels = (rlmMaterialChannel*)MemAlloc(extraChannelCount * sizeof(rlmMaterialChannel));

    if (mat->has_pbr_metallic_roughness)
    {
        LoadPBRMaterial(mat, material);
        nextChannel = 2;
    }
    else if (mat->has_pbr_specular_glossiness)
    {

    }

    // Load normal texture
    if (mat->normal_texture.texture)
    {
        Image imNormal = LoadImageFromCgltfImage(mat->normal_texture.texture->image, "");
        if (imNormal.data != NULL)
        {
            material->extraChannels[nextChannel].color = WHITE;

            material->extraChannels[nextChannel].textureSlot = MATERIAL_MAP_NORMAL;
            material->extraChannels[nextChannel].textureLoc = material->shader.locs[SHADER_LOC_MAP_NORMAL];

            material->extraChannels[nextChannel].ownsTexture = true;
            material->extraChannels[nextChannel].textureId = LoadTextureFromImage(imNormal).id;

            nextChannel++;
            UnloadImage(imNormal);
        }
    }

    // Load ambient occlusion texture
    if (mat->occlusion_texture.texture)
    {
        Image imOcclusion = LoadImageFromCgltfImage(mat->occlusion_texture.texture->image, "");
        if (imOcclusion.data != NULL)
        {
            material->extraChannels[nextChannel].color = WHITE;

            material->extraChannels[nextChannel].textureSlot = MATERIAL_MAP_OCCLUSION;
            material->extraChannels[nextChannel].textureLoc = material->shader.locs[SHADER_LOC_MAP_NORMAL];

            material->extraChannels[nextChannel].ownsTexture = true;
            material->extraChannels[nextChannel].textureId = LoadTextureFromImage(imOcclusion).id;

            nextChannel++;
            UnloadImage(imOcclusion);
        }
    }

    // Load emissive texture
    if (mat->emissive_texture.texture)
    {
        Image imEmissive = LoadImageFromCgltfImage(mat->emissive_texture.texture->image, "");
        if (imEmissive.data != NULL)
        {
            material->extraChannels[nextChannel].color.r = (unsigned char)(mat->emissive_factor[0] * 255);
            material->extraChannels[nextChannel].color.g = (unsigned char)(mat->emissive_factor[1] * 255);
            material->extraChannels[nextChannel].color.b = (unsigned char)(mat->emissive_factor[2] * 255);
            material->extraChannels[nextChannel].color.a = 255;

            material->extraChannels[nextChannel].textureSlot = MATERIAL_MAP_EMISSION;
            material->extraChannels[nextChannel].textureLoc = material->shader.locs[SHADER_LOC_MAP_NORMAL];

            material->extraChannels[nextChannel].ownsTexture = true;
            material->extraChannels[nextChannel].textureId = LoadTextureFromImage(imEmissive).id;
            UnloadImage(imEmissive);
        }
    }
}

void LoadGroups(cgltf_data* data, rlmModel* model)
{
    if (data->materials_count == 0)
    {
        model->groupCount = 1;
        model->groups = (rlmModelGroup*)MemAlloc(model->groupCount * sizeof(rlmModelGroup));
        model->groups[0].material = rlmGetDefaultMaterial();
    }
    else
    {
        model->groupCount = (int)data->materials_count;
        model->groups = (rlmModelGroup*)MemAlloc(model->groupCount * sizeof(rlmModelGroup));

        for (int m = 0; m < data->materials_count; m++)
        {
            cgltf_material* mat = &(data->materials[m]);
            model->groups[m].material = rlmGetDefaultMaterial();

            LoadMaterial(mat, &model->groups[m].material);
        }
    }
}

#define LOAD_ATTRIBUTE(accesor, numComp, srcType, dstPtr) LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, srcType)

#define LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, dstType) \
    { \
        int n = 0; \
        srcType *buffer = (srcType *)accesor->buffer_view->buffer->data + accesor->buffer_view->offset/sizeof(srcType) + accesor->offset/sizeof(srcType); \
        for (unsigned int k = 0; k < accesor->count; k++) \
        {\
            for (int l = 0; l < numComp; l++) \
            {\
                dstPtr[numComp*k + l] = (dstType)buffer[n + l];\
            }\
            n += (int)(accesor->stride/sizeof(srcType));\
        }\
    }


void ProcessGLTFPositionPrimitive(rlmMesh* mesh, cgltf_accessor* attribute, Matrix* worldMatrix)
{
    if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_32f))
    {
        // Init raylib mesh vertices to copy glTF attribute data
        mesh->meshBuffers->vertexCount = (int)attribute->count;
        if (mesh->meshBuffers->vertices == NULL)
            mesh->meshBuffers->vertices = (float*)MemAlloc((unsigned int)(attribute->count * 3 * sizeof(float)));

        // Load 3 components of float data type into mesh.vertices
        LOAD_ATTRIBUTE(attribute, 3, float, mesh->meshBuffers->vertices)

        // Transform the vertices
        float* vertices = mesh->meshBuffers->vertices;
        for (unsigned int k = 0; k < attribute->count; k++)
        {
            Vector3 vt = Vector3Transform((Vector3){ vertices[3 * k], vertices[3 * k + 1], vertices[3 * k + 2] }, *worldMatrix);
            vertices[3 * k] = vt.x;
            vertices[3 * k + 1] = vt.y;
            vertices[3 * k + 2] = vt.z;
        }
    }
}

void ProcessGLTFNormalPrimitive(rlmMesh* mesh, cgltf_accessor* attribute, Matrix* worldMatrix)
{
    if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_32f))
    {
        // Init raylib mesh normals to copy glTF attribute data
        if (mesh->meshBuffers->normals == NULL)
            mesh->meshBuffers->normals = (float*)MemAlloc((unsigned int)(attribute->count * 3 * sizeof(float)));

        // Load 3 components of float data type into mesh.normals
        LOAD_ATTRIBUTE(attribute, 3, float, mesh->meshBuffers->normals)

            // Transform the normals
            float* normals = mesh->meshBuffers->normals;

        Matrix normalMatrix = MatrixTranspose(MatrixInvert(*worldMatrix));
        for (unsigned int k = 0; k < attribute->count; k++)
        {
            Vector3 nt = Vector3Transform((Vector3){ normals[3 * k], normals[3 * k + 1], normals[3 * k + 2] }, normalMatrix);
            normals[3 * k] = nt.x;
            normals[3 * k + 1] = nt.y;
            normals[3 * k + 2] = nt.z;
        }
    }
}

void ProcessGLTFTangentPrimitive(rlmMesh* mesh, cgltf_accessor* attribute, Matrix* worldMatrix)
{
    if ((attribute->type == cgltf_type_vec4) && (attribute->component_type == cgltf_component_type_r_32f))
    {
        // Init raylib mesh tangent to copy glTF attribute data
        if (mesh->meshBuffers->tangents == NULL)
            mesh->meshBuffers->tangents = (float*)MemAlloc((unsigned int)(attribute->count * 4 * sizeof(float)));

        // Load 4 components of float data type into mesh.tangents
        LOAD_ATTRIBUTE(attribute, 4, float, mesh->meshBuffers->tangents)

        // Transform the tangents
        float* tangents = mesh->meshBuffers->tangents;
        for (unsigned int k = 0; k < attribute->count; k++)
        {
            Vector3 tt = Vector3Transform((Vector3){ tangents[3 * k], tangents[3 * k + 1], tangents[3 * k + 2] }, *worldMatrix);
            tangents[3 * k] = tt.x;
            tangents[3 * k + 1] = tt.y;
            tangents[3 * k + 2] = tt.z;
        }
    }
}

void ProcessGLTFTextureCoordsPrimitive(rlmMesh* mesh, cgltf_accessor* attribute, int index)
{
    if (attribute->type != cgltf_type_vec2)
        return;

    // Support up to 2 texture coordinates attributes
    float* texcoordPtr = NULL;

    if (index == 0)
    {
        if (mesh->meshBuffers->texcoords == NULL)
            mesh->meshBuffers->texcoords = (float*)MemAlloc((unsigned int)(attribute->count * 2 * sizeof(float)));
        texcoordPtr = mesh->meshBuffers->texcoords;
    }
    else if (index == 1)
    {
        if (mesh->meshBuffers->texcoords2 == NULL)
            mesh->meshBuffers->texcoords2 = (float*)MemAlloc((unsigned int)(attribute->count * 2 * sizeof(float)));
        texcoordPtr = mesh->meshBuffers->texcoords2;
    }
    else
    {
        return;
    }

    if (attribute->component_type == cgltf_component_type_r_32f)  // vec2, float
    {
        // Load 3 components of float data type into mesh.texcoords
        LOAD_ATTRIBUTE(attribute, 2, float, texcoordPtr)
    }
    else if (attribute->component_type == cgltf_component_type_r_8u) // vec2, u8n
    {
        // Load data into a temp buffer to be converted to raylib data type
        unsigned char* temp = (unsigned char*)MemAlloc((unsigned int)(attribute->count * 2 * sizeof(unsigned char)));
        LOAD_ATTRIBUTE(attribute, 2, unsigned char, temp);

        // Convert data to raylib texcoord data type (float)
        for (unsigned int t = 0; t < attribute->count * 2; t++)
            texcoordPtr[t] = (float)temp[t] / 255.0f;

        MemFree(temp);
    }
    else if (attribute->component_type == cgltf_component_type_r_16u) // vec2, u16n
    {
        // Load data into a temp buffer to be converted to raylib data type
        unsigned short* temp = (unsigned short*)MemAlloc((unsigned int)(attribute->count * 2 * sizeof(unsigned short)));
        LOAD_ATTRIBUTE(attribute, 2, unsigned short, temp);

        // Convert data to raylib texcoord data type (float)
        for (unsigned int t = 0; t < attribute->count * 2; t++)
            texcoordPtr[t] = (float)temp[t] / 65535.0f;

        MemFree(temp);
    }
}

void ProcessGLTFColorPrimitive(rlmMesh* mesh, cgltf_accessor* attribute)
{
    // WARNING: SPECS: All components of each COLOR_n accessor element MUST be clamped to [0.0, 1.0] range

    if (mesh->meshBuffers->colors == NULL)
        mesh->meshBuffers->colors = (unsigned char*)MemAlloc((unsigned int)(attribute->count * 4 * sizeof(unsigned char)));

    if (attribute->type == cgltf_type_vec3)  // RGB
    {
        if (attribute->component_type == cgltf_component_type_r_8u)
        {
            // Load data into a temp buffer to be converted to raylib data type
            unsigned char* temp = (unsigned char*)MemAlloc((unsigned int)(attribute->count * 3 * sizeof(unsigned char)));
            LOAD_ATTRIBUTE(attribute, 3, unsigned char, temp);

            // Convert data to raylib color data type (4 bytes)
            for (unsigned int c = 0, k = 0; c < (attribute->count * 4 - 3); c += 4, k += 3)
            {
                mesh->meshBuffers->colors[c] = temp[k];
                mesh->meshBuffers->colors[c + 1] = temp[k + 1];
                mesh->meshBuffers->colors[c + 2] = temp[k + 2];
                mesh->meshBuffers->colors[c + 3] = 255;
            }

            MemFree(temp);
        }
        else if (attribute->component_type == cgltf_component_type_r_16u)
        {
            // Load data into a temp buffer to be converted to raylib data type
            unsigned short* temp = (unsigned short*)MemAlloc((unsigned int)(attribute->count * 3 * sizeof(unsigned short)));
            LOAD_ATTRIBUTE(attribute, 3, unsigned short, temp);

            // Convert data to raylib color data type (4 bytes)
            for (unsigned int c = 0, k = 0; c < (attribute->count * 4 - 3); c += 4, k += 3)
            {
                mesh->meshBuffers->colors[c] = (unsigned char)(((float)temp[k] / 65535.0f) * 255.0f);
                mesh->meshBuffers->colors[c + 1] = (unsigned char)(((float)temp[k + 1] / 65535.0f) * 255.0f);
                mesh->meshBuffers->colors[c + 2] = (unsigned char)(((float)temp[k + 2] / 65535.0f) * 255.0f);
                mesh->meshBuffers->colors[c + 3] = 255;
            }

            MemFree(temp);
        }
        else if (attribute->component_type == cgltf_component_type_r_32f)
        {
            // Load data into a temp buffer to be converted to raylib data type
            float* temp = (float*)MemAlloc((unsigned int)(attribute->count * 3 * sizeof(float)));
            LOAD_ATTRIBUTE(attribute, 3, float, temp);

            // Convert data to raylib color data type (4 bytes)
            for (unsigned int c = 0, k = 0; c < (attribute->count * 4 - 3); c += 4, k += 3)
            {
                mesh->meshBuffers->colors[c] = (unsigned char)(temp[k] * 255.0f);
                mesh->meshBuffers->colors[c + 1] = (unsigned char)(temp[k + 1] * 255.0f);
                mesh->meshBuffers->colors[c + 2] = (unsigned char)(temp[k + 2] * 255.0f);
                mesh->meshBuffers->colors[c + 3] = 255;
            }

            MemFree(temp);
        }
    }
    else if (attribute->type == cgltf_type_vec4) // RGBA
    {
        if (attribute->component_type == cgltf_component_type_r_8u)
        {
            // Load 4 components of unsigned char data type into mesh.colors
            LOAD_ATTRIBUTE(attribute, 4, unsigned char, mesh->meshBuffers->colors)
        }
        else if (attribute->component_type == cgltf_component_type_r_16u)
        {
            // Load data into a temp buffer to be converted to raylib data type
            unsigned short* temp = (unsigned short*)MemAlloc((unsigned int)(attribute->count * 4 * sizeof(unsigned short)));
            LOAD_ATTRIBUTE(attribute, 4, unsigned short, temp);

            // Convert data to raylib color data type (4 bytes)
            for (unsigned int c = 0; c < attribute->count * 4; c++)
                mesh->meshBuffers->colors[c] = (unsigned char)(((float)temp[c] / 65535.0f) * 255.0f);

            MemFree(temp);
        }
        else if (attribute->component_type == cgltf_component_type_r_32f)
        {
            // Load data into a temp buffer to be converted to raylib data type
            float* temp = (float*)MemAlloc((unsigned int)(attribute->count * 4 * sizeof(float)));
            LOAD_ATTRIBUTE(attribute, 4, float, temp);

            // Convert data to raylib color data type (4 bytes), we expect the color data normalized
            for (unsigned int c = 0; c < attribute->count * 4; c++)
                mesh->meshBuffers->colors[c] = (unsigned char)(temp[c] * 255.0f);

            MemFree(temp);
        }
    }
}

void LoadGLTFMeshIndecies(rlmMesh* mesh, cgltf_primitive* prim)
{
    // Load primitive indices data (if provided)
    if ((prim->indices == NULL) || (prim->indices->buffer_view == NULL))
    {
        mesh->gpuMesh.isIndexed = false;
        mesh->meshBuffers->triangleCount = mesh->meshBuffers->vertexCount / 3;    // Unindexed mesh
        mesh->gpuMesh.elementCount = mesh->meshBuffers->vertexCount;
        return;
    }

    mesh->gpuMesh.isIndexed = true;

    cgltf_accessor* attribute = prim->indices;

    mesh->meshBuffers->triangleCount = (int)attribute->count / 3;

    mesh->gpuMesh.elementCount = mesh->meshBuffers->triangleCount * 3;

    // Init raylib mesh indices to copy glTF attribute data
    if (mesh->meshBuffers->indices == NULL)
        mesh->meshBuffers->indices = (unsigned short*)MemAlloc((unsigned int)(attribute->count * sizeof(unsigned short)));

    if (attribute->component_type == cgltf_component_type_r_16u)
    {
        // Load unsigned short data type into mesh.indices
        LOAD_ATTRIBUTE(attribute, 1, unsigned short, mesh->meshBuffers->indices)
    }
    else if (attribute->component_type == cgltf_component_type_r_8u)
    {
        LOAD_ATTRIBUTE_CAST(attribute, 1, unsigned char, mesh->meshBuffers->indices, unsigned short)

    }
    else if (attribute->component_type == cgltf_component_type_r_32u)
    {
        LOAD_ATTRIBUTE_CAST(attribute, 1, unsigned int, mesh->meshBuffers->indices, unsigned short);
    }
}

void LoadMeshes(cgltf_data* data, rlmModelGroup* group, bool keepCPUData)
{
    group->meshCount = 0;
    for (int index = 0; index < data->nodes_count; index++)
    {
        cgltf_node* node = data->nodes + index;

        cgltf_mesh* mesh = node->mesh;
        if (!mesh)
            continue;

        for (int primIndex = 0; primIndex < mesh->primitives_count; primIndex++)
        {
            cgltf_primitive* prim = mesh->primitives + primIndex;

            if (strcmp(prim->material->name, group->material.name) != 0)
                continue;

            if (prim->type != cgltf_primitive_type_triangles)
                continue;

            group->meshCount++;
        }
    }  
    group->meshes = (rlmMesh*)MemAlloc(group->meshCount * sizeof(rlmMesh));

    group->ownsMeshes = true;
    group->ownsMeshList = true;
    group->meshDisableFlags = (bool*)MemAlloc(group->meshCount * sizeof(bool));

    int meshIndex = 0;
    for (int index = 0; index < data->nodes_count; index++)
    {
        cgltf_node* node = data->nodes + index;

        cgltf_mesh* mesh = node->mesh;
        if (!mesh)
            continue;

        rlmMesh* pOutMesh = group->meshes + meshIndex;

        cgltf_float worldTransform[16];
        cgltf_node_transform_world(node, worldTransform);

        Matrix worldMatrix = {
            worldTransform[0], worldTransform[4], worldTransform[8], worldTransform[12],
            worldTransform[1], worldTransform[5], worldTransform[9], worldTransform[13],
            worldTransform[2], worldTransform[6], worldTransform[10], worldTransform[14],
            worldTransform[3], worldTransform[7], worldTransform[11], worldTransform[15]
        };

        Matrix worldMatrixNormals = MatrixTranspose(MatrixInvert(worldMatrix));

        for (int primIndex = 0; primIndex < mesh->primitives_count; primIndex++)
        {
            cgltf_primitive* prim = mesh->primitives + primIndex;

            if (strcmp(prim->material->name, group->material.name) != 0)
                continue;

            if (prim->type != cgltf_primitive_type_triangles)
                continue;

            const char* temp = TextFormat("%s.%s", node->name, mesh->name);
            pOutMesh->name = MemAlloc((int)strlen(temp) + 1);
            strcpy(pOutMesh->name, temp);

            pOutMesh->transform = rlmPQSIdentity();
            pOutMesh->meshBuffers = (rlmMeshBuffers*)MemAlloc(sizeof(rlmMeshBuffers));


            for (unsigned int j = 0; j < mesh->primitives[primIndex].attributes_count; j++)
            {
                // Check the different attributes for every primitive
                if (mesh->primitives[primIndex].attributes[j].type == cgltf_attribute_type_position)      // POSITION, vec3, float
                {
                    ProcessGLTFPositionPrimitive(pOutMesh, mesh->primitives[primIndex].attributes[j].data, &worldMatrix);
                }
                else if (mesh->primitives[primIndex].attributes[j].type == cgltf_attribute_type_normal)   // NORMAL, vec3, float
                {
                    ProcessGLTFNormalPrimitive(pOutMesh, mesh->primitives[primIndex].attributes[j].data, &worldMatrix);
                }
                else if (mesh->primitives[primIndex].attributes[j].type == cgltf_attribute_type_tangent)   // TANGENT, vec3, float
                {
                    ProcessGLTFTangentPrimitive(pOutMesh, mesh->primitives[primIndex].attributes[j].data, &worldMatrix);
                }
                else if (mesh->primitives[primIndex].attributes[j].type == cgltf_attribute_type_texcoord) // TEXCOORD_n, vec2, float/u8n/u16n
                {
                    ProcessGLTFTextureCoordsPrimitive(pOutMesh, mesh->primitives[primIndex].attributes[j].data, mesh->primitives[primIndex].attributes[j].index);
                }
                else if (mesh->primitives[primIndex].attributes[j].type == cgltf_attribute_type_color)    // COLOR_n, vec3/vec4, float/u8n/u16n
                {
                    ProcessGLTFColorPrimitive(pOutMesh, mesh->primitives[primIndex].attributes[j].data);
                }
                // NOTE: Attributes related to animations are processed separately
            }

            LoadGLTFMeshIndecies(pOutMesh, mesh->primitives + primIndex);

            rlmUploadMesh(pOutMesh, !keepCPUData);

            meshIndex++;
        }
    }
}

rlmModel rlmLoadModelGLTF(const char* fileName, bool keepCPUData)
{
    // glTF file loading
    int dataSize = 0;
    unsigned char* fileData = LoadFileData(fileName, &dataSize);

    rlmModel model = { 0 };
    model.orientationTransform = rlmPQSIdentity();

    if (fileData == NULL)
        return model;

    // glTF data loading
    cgltf_options options = {};
    options.file.read = LoadFileGLTFCallback;
    options.file.release = ReleaseFileGLTFCallback;
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

    if (result == cgltf_result_success)
    {
        // Force reading data buffers (fills buffer_view->buffer->data)
        // NOTE: If an uri is defined to base64 data or external path, it's automatically loaded
        result = cgltf_load_buffers(&options, data, fileName);
        if (result == cgltf_result_success)
        {
            int primitivesCount = GetGLTFPrimCount(data);

            LoadGroups(data, &model);

            for (int g = 0; g < model.groupCount; g++)
            {
                LoadMeshes(data, model.groups + g, keepCPUData);
            }
        }
        // Free all cgltf loaded data
        cgltf_free(data);
    }

    // WARNING: cgltf requires the file pointer available while reading data
    UnloadFileData(fileData);

    return model;
}