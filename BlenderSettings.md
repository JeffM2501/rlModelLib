# Best practices to get models out of blender into raylib
Getting models and animations out of blender into raylib with best results, requires understandig the limitations of raylib's animation system, and getting blender setup to provide the best possible data for raylib to read.

## Work in progress
This document is a work in progress and is under revisison.

# Blender Version
Use the current version of blender when possible. Older versions of blender do not have all the required options needed to correclty export to some formats.

#Animation Limitations
Raylib has a relativly simple animation system that has several limitaitons.

1) Bones only. Raylib can only animate bones, it can not animate mesh objects directly. If you wish to animate entire objects by transforms, you must create an armature, and attach the meshes as children of the bones in the armature. Then animate the bones, not the meshes. Raylib will attach the vertecies of the meshes to the bones and animate them.

2) No Morph Targets. Raylib can not do vertex based animation, just bones, so all animations must be done via bones, morph targets will not work.

4) No Animated TextureCoordiantes or UVs. Raylib can only animate bones and attached vertex data, it can not animate texture coordinates.

# Using GLTF
GLTF/GLB is a great format, but unfortonately supports more features that raylib does, so it is very easy to get files that will not import properly  into raylib. 
To get files to work in raylib, you need to do a few specific thing when you export from blend.

1) No more than 4 bones can be assigned to any one vertex. GLTF supports 8 bones per vertex, but raylib only supports 4. This is controlled in the Data->Armature-> Skinning -> Include All Bone Influences in the export options and must be unchecked.

2) Ensure that the T pos armature is used for the rest postion. Raylib builds all animations from the T pose so the armature for the binding pose must match. This is controlled by the Data -> Armature -> Use Rest Posiition Armature, and must be checked. This is one of the biggest reasons why animations 'come in wierd' from files from the internet. GLTF files that may need to be re-exported with this option checked to work correctly.

3) No armature scale. The armature can't be scaled, it must match up to the T pose in order to work. Validate that the armature is not scaled.

4) Apply Modifiers. All modifers must be baked into the exported geometry, raylib can not apply them for you at runtime. When exporting ensure that the the Mesh->Apply Modifiers is checked.