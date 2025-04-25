/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

-- Copyright (c) 2020-2024 Jeffery Myers
--
--This software is provided "as-is", without any express or implied warranty. In no event 
--will the authors be held liable for any damages arising from the use of this software.

--Permission is granted to anyone to use this software for any purpose, including commercial 
--applications, and to alter it and redistribute it freely, subject to the following restrictions:

--  1. The origin of this software must not be misrepresented; you must not claim that you 
--  wrote the original software. If you use this software in a product, an acknowledgment 
--  in the product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented
--  as being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.

*/

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "rlModels.h"	
#include "rlModels_IO.h"

Camera3D ViewCam = { 0 };

rlmModel masterRobotModel = { 0 };
rlmModelAnimationSet animSet;

rlmAnimatedModelInstance animInstance;

//rlmRelativeAnimationKeyframe relativeKeyframe;

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);

    ViewCam.fovy = 45;
    ViewCam.position.z = -300;
    ViewCam.position.y = 80;
    ViewCam.target.y = 80;
    ViewCam.up.y = 1;

    rlSetClipPlanes(1, 5000);
    rlSetLineWidth(4);

    Model raylibModel = LoadModel("resources/bot.glb");
  
    Shader shader = LoadShader("resources/skinning.vs", "resources/skinning.fs");
   
    masterRobotModel = rlmLoadFromModel(raylibModel);

    for (int i = 0; i < masterRobotModel.groupCount; i++)
        rlmSetMaterialDefShader(&masterRobotModel.groups[i].material, shader);

    if (masterRobotModel.skeleton)
    {
        ModelAnimation* animations = LoadModelAnimations("resources/bot.aim.glb", &animSet.sequenceCount);
        animSet.sequences = rlmLoadModelAnimations(masterRobotModel.skeleton, animations, animSet.sequenceCount);
    }

    animInstance.model = &masterRobotModel;
    animInstance.sequences = &animSet;
    animInstance.currentPose = rlmLoadPoseFromModel(masterRobotModel);

    animInstance.transform = rlmPQSIdentity();

    rlmSetAnimationInstanceSequence(&animInstance, 0);

    //relativeKeyframe = rlmLoadRelativeKeyframe(animInstance.model->skeleton, animSet.sequences[0].keyframes[0]);
}

void GameCleanup()
{
    rlmUnloadModel(&masterRobotModel);
    CloseWindow();
}

bool GameUpdate()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        UpdateCamera(&ViewCam, CAMERA_THIRD_PERSON);

    rlmAdvanceAnimationInstance(&animInstance, GetFrameTime());

    if (masterRobotModel.skeleton)
    {
        if (IsKeyPressed(KEY_LEFT))
        {
            animInstance.currentSequence--;
            if (animInstance.currentSequence < 0)
                animInstance.currentSequence = animInstance.sequences->sequenceCount;

            rlmSetAnimationInstanceSequence(&animInstance, animInstance.currentSequence);
        }

        if (IsKeyPressed(KEY_RIGHT))
        {
            animInstance.currentSequence++;
            if (animInstance.currentSequence >= animInstance.sequences->sequenceCount)
                animInstance.currentSequence = 0;

            rlmSetAnimationInstanceSequence(&animInstance, animInstance.currentSequence);
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            animInstance.interpolate = !animInstance.interpolate;
        }
    }
   
    return true;
}

void DrawBone(rlmBoneInfo* bone, const rlmPQSTransorm& parentTransform, rlmAnimationKeyframe &keyframe)
{
    DrawLine3D(parentTransform.position, keyframe.boneTransforms[bone->boneId].position, DARKGREEN);
    DrawSphereWires(keyframe.boneTransforms[bone->boneId].position, 0.05f, 5, 5, GREEN);

    for (int i = 0; i < bone->childCount; i++)
        DrawBone(bone->childBones[i], keyframe.boneTransforms[bone->boneId], keyframe);
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    BeginMode3D(ViewCam);

    //DrawModel(raylibModel, Vector3Zeros, 1, WHITE);

    rlmDrawModelWithPose(*animInstance.model, animInstance.transform, &animInstance.currentPose);
    rlDrawRenderBatchActive();

    rlDisableDepthMask();
    rlDisableDepthTest();

    DrawBone(animInstance.model->skeleton->rootBone, animInstance.transform, animInstance.sequences->sequences[animInstance.currentSequence].keyframes[animInstance.currentFrame]);
  //  DrawRelativeBone(animInstance.model->skeleton->rootBone, animInstance.transform, relativeKeyframe);

    rlDrawRenderBatchActive();
    rlEnableDepthTest();
    rlEnableDepthMask();

    DrawGrid(100, 10);

    EndMode3D();

    EndDrawing();
}

int main()
{
    GameInit();

    while (!WindowShouldClose())
    {
        if (!GameUpdate())
            break;

        GameDraw();
    }
    GameCleanup();

    return 0;
}