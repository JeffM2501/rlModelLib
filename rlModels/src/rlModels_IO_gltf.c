#include "external/cgltf.h"
#include "raylib.h"
#include "rlModels.h"

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
    material->baseChannel.textureId = 0;
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

            unsigned char metalnessValue = mat->pbr_metallic_roughness.metallic_factor * 255;
            material->extraChannels[0].color = (Color){ metalnessValue,metalnessValue,metalnessValue,255 };

            material->extraChannels[0].textureSlot = MATERIAL_MAP_METALNESS;
            material->extraChannels[0].textureLoc = material->shader.locs[SHADER_LOC_MAP_METALNESS];

            material->extraChannels[0].ownsTexture = true;
            material->extraChannels[0].textureId = LoadTextureFromImage(imMetallic).id;


            unsigned char roughnessValue = mat->pbr_metallic_roughness.roughness_factor * 255;
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
        model->groupCount = data->materials_count;
        model->groups = (rlmModelGroup*)MemAlloc(model->groupCount * sizeof(rlmModelGroup));

        for (int m = 0; m < data->materials_count; m++)
        {
            cgltf_material* mat = &(data->materials[m]);
           
            model->groups[m].material.name = MemAlloc(strlen(mat->name) + 1);
            strcpy(model->groups[m].material.name, mat->name);

            LoadMaterial(mat, &model->groups[m].material);
        }
    }
}

void LoadMeshes(cgltf_data* data, rlmModelGroup* group)
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

            group->meshCount++;
        }
    }  
    group->meshes = (rlmMesh*)MemAlloc(group->meshCount * sizeof(rlmMesh));

    int meshIndex = 0;
    for (int index = 0; index < data->nodes_count; index++)
    {
        cgltf_node* node = data->nodes + index;

        cgltf_mesh* mesh = node->mesh;
        if (!mesh)
            continue;

        rlmMesh* pOutMesh = group->meshes + meshIndex;

        for (int primIndex = 0; primIndex < mesh->primitives_count; primIndex++)
        {
            cgltf_primitive* prim = mesh->primitives + primIndex;

            if (strcmp(prim->material->name, group->material.name) != 0)
                continue;

            pOutMesh->name = MemAlloc(strlen(mesh->name) + 1);
            strcpy(pOutMesh->name, mesh->name);

            pOutMesh->transform = rlmPQSIdentity();
            pOutMesh->meshBuffers = (rlmMeshBuffers*)MemAlloc(sizeof(rlmMeshBuffers));

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
                LoadMeshes(data, &model.groups + g);
            }
        }
        // Free all cgltf loaded data
        cgltf_free(data);
    }

    // WARNING: cgltf requires the file pointer available while reading data
    UnloadFileData(fileData);

    return model;
}