# rlModelLib
A drop in libary for raylib that implements more advanced and robust 3d model and animation support.

# Problems With Raylib model
* Meshes alway store geometry data
* Meshes duplicate pose matrix
* Drawing does multiple calls to the same shader/ uniforms
* No way to override materials
* No way to do inerpolation
* No way to do relative skeletal animations
* Multiple instances of meshes require many assignments.
* No clear ownership of textures and shaders

# Design goals


# Structures

rlmGPUMesh	// data needed to draw a mesh
{
	VAO
	VBOS[]
} // 12 bytes

rlmMeshBuffers	// geometry buffers used to define a mesh
{
	Verts[]
	Normals[]
	TextureCoords[]
	Colors[]
	TextureCoord2s[]
	Tangents[]
	BoneWeights[]
	BoneIds[]
	Indexes[]
}// 9*size_t bytes

rlmMesh // a mesh
{
	rlmGPUMesh GPUMesh;
	rlmMeshBuffers* MeshBuffers = nullptr; // optional adter upload
}; //20 bytes

rlmMaterialChannel // a texture map in a material
{
	int TextureId;	//4
	bool OwnsTexture; //1
	int ShaderLoc;	//4
}; // 9 bytes

rlmMaterialDef // a shader and it's input texture
{
	Shader shader;	// 12
	bool OwnsShader; //1
	
	rlmMaterialChannel	BaseChannel; //9
	Color Tint;		//4
	int MaterialChannels;			//4
	
	rlmMaterialChannel ExtraChannes[0];// 8
} // 38 bytes

rlmModelGroup // a group of meshes that share a material
{
	rlmMaterialDef Material;
	int MeshCount;
	rlmMesh Meshes[];
}; //50 bytes

rlmPQSTransorm // a transform
{
	Vector3 Translation;
	Quaterneion Rotation;
	Vector3 Scale;
} 40 bytes

rlmModel // a group of meshes, materials, and an orientation transform
{
	int GroupCount;	// 4
	rlmModelGroup Groups[]; // 8
	bool OwnsGroups;	//1
	rlmPQSTransorm OrientationTransform; // 40 (store as pointer? )
}; // 53 bytes

void rlmUploadMesh(rlmMesh* mesh, bool releaseGeoBuffers);

rlmMaterialDef rlmGetDefaultMaterial();
void rlmFreeMaterial(rlmMaterialDef*);

void rlmAddMaterialChannel(rlmMaterialDef*, Texture2d, int shaderLoc)

void rlmSetMaterialChannelTexture(rlmMaterialChannel*, Texture2D);

void rlmSetMaterialDefShader(rlmMaterialDef*, Shader);