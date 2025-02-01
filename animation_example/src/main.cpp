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
#include "raymath.h"

#include "rlModels.h"	
#include "rlModels_IO.h"

Camera3D ViewCam = { 0 };

rlmModel newModel = { 0 };

Model raylibModel = { 0 };

rlmModelAnimationPose pose = { 0 };
rlmModelAniamtionSequence* sequences = NULL;

rlmBoneInfo* blendBone = NULL;

float accumumulator = 0;

int currentSequence = 10;

int topSequence = 5;

int currentFrame = 0;
int currentTopFrame = 0;
rlmModelAnimationPose topPose = { 0 };

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);

    ViewCam.fovy = 45;
    ViewCam.position.z = -10;
    ViewCam.position.y = 2;
    ViewCam.target.y = 1;
    ViewCam.up.y = 1;

    raylibModel = LoadModel("resources/robot.glb");
  
    Shader shader = LoadShader("resources/skinning.vs", "resources/skinning.fs");
   
    newModel = rlmLoadFromModel(raylibModel);

    for (int i = 0; i < newModel.groupCount; i++)
        rlmSetMaterialDefShader(&newModel.groups[i].material, shader);

    if (newModel.skeleton)
    {
        int animationCount = 0;
        ModelAnimation* animations = LoadModelAnimations("resources/robot.glb", &animationCount);
        sequences = rlmLoadModelAnimations(newModel.skeleton, animations, animationCount);
        pose = rlmLoadPoseFromModel(newModel);
        topPose = rlmLoadPoseFromModel(newModel);

        rlmSetPoseToKeyframe(newModel, &pose, sequences[currentSequence].keyframes[0]);
        rlmSetPoseToKeyframe(newModel, &topPose, sequences[topSequence].keyframes[0]);

        blendBone = rlmFindBoneByName(newModel, "Abdomen");
    }
}

void GameCleanup()
{
    rlmUnloadModel(&newModel);
    CloseWindow();
}

bool GameUpdate()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        UpdateCamera(&ViewCam, CAMERA_THIRD_PERSON);

    accumumulator += GetFrameTime();

    float invFPS = 1.0f / sequences[0].fps;

    //invFPS *= 4;

    while (accumumulator >= invFPS)
    {
        accumumulator -= invFPS;

        currentFrame++;
        if (currentFrame >= sequences[currentSequence].keyframeCount)
            currentFrame = 0;

        currentTopFrame++;
        if (currentTopFrame >= sequences[topSequence].keyframeCount)
            currentTopFrame = 0;
    }

    int nextFrame = currentFrame+1;
    if (nextFrame >= sequences[currentSequence].keyframeCount)
        nextFrame = 0;

    rlmSetPoseToKeyframesLerp(newModel, &pose, sequences[currentSequence].keyframes[currentFrame], sequences[currentSequence].keyframes[currentFrame], accumumulator/invFPS);
    
    nextFrame = currentTopFrame + 1;
    if (nextFrame >= sequences[topSequence].keyframeCount)
        nextFrame = 0;

    
    //rlmSetPoseToKeyframe(newModel, &pose, sequences[0].keyframes[currentFrame]);
    rlmSetPoseToKeyframesLerpEx(newModel, &pose, sequences[topSequence].keyframes[currentTopFrame], sequences[topSequence].keyframes[nextFrame], accumumulator / invFPS, blendBone);

    return true;
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    BeginMode3D(ViewCam);

  //DrawModel(raylibModel, Vector3Zeros, 1, WHITE);

    rlmDrawModelWithPose(newModel, rlmPQSIdentity(), &pose);

    DrawGrid(100, 1);

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