/*
Raylib Model Lib example

-- Copyright (c) 2025 Jeffery Myers
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

rlmModel masterRobotModel = { 0 };

rlmModel coloredRobots[5];

rlmModelAnimationSet animSet;

rlmAnimatedModelInstance modelInstance[5];

void SetupClones()
{
    coloredRobots[0] = rlmCloneModel(masterRobotModel);
    coloredRobots[0].groups[1].material.baseChannel.color = DARKBLUE;

    coloredRobots[1] = rlmCloneModel(masterRobotModel);
    coloredRobots[1].groups[1].material.baseChannel.color = RED;

    coloredRobots[2] = rlmCloneModel(masterRobotModel);

    coloredRobots[3] = rlmCloneModel(masterRobotModel);
    coloredRobots[3].groups[1].material.baseChannel.color = PURPLE;

    coloredRobots[4] = rlmCloneModel(masterRobotModel);
    coloredRobots[4].groups[1].material.baseChannel.color = DARKGREEN;
}

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);

    ViewCam.fovy = 45;
    ViewCam.position.z = -10;
    ViewCam.position.y = 4;
    ViewCam.target.y = 2;
    ViewCam.up.y = 1;

    Model raylibModel = LoadModel("resources/robot.glb");
  
    Shader shader = LoadShader("resources/skinning.vs", "resources/skinning.fs");
   
    masterRobotModel = rlmLoadFromModel(raylibModel);

    for (int i = 0; i < masterRobotModel.groupCount; i++)
        rlmSetMaterialDefShader(&masterRobotModel.groups[i].material, shader);

    if (masterRobotModel.skeleton)
    {
        ModelAnimation* animations = LoadModelAnimations("resources/robot.glb", &animSet.sequenceCount);
        animSet.sequences = rlmLoadModelAnimations(masterRobotModel.skeleton, animations, animSet.sequenceCount);
    }

    SetupClones();

    for (int i = 0; i < 5; i++)
    {
        modelInstance[i].model = &coloredRobots[i];
        modelInstance[i].transform = rlmPQSTranslation(-6.75f + (i * 3.5f), 0,0);
        modelInstance[i].transform.rotation = QuaternionFromAxisAngle(Vector3UnitY, 180 * DEG2RAD);

        if (masterRobotModel.skeleton)
        {
            modelInstance[i].sequences = &animSet;

            modelInstance[i].interpolate = true;

            modelInstance[i].currentPose = rlmLoadPoseFromModel(masterRobotModel);

            rlmSetAnimationInstanceSequence(&modelInstance[i], i);
        }
    }
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

    for (int i = 0; i < 5; i++)
        rlmAdvanceAnimationInstance(&modelInstance[i], GetFrameTime());

    if (masterRobotModel.skeleton)
    {
        if (IsKeyPressed(KEY_LEFT))
        {
            modelInstance[2].currentSequence--;
            if (modelInstance[2].currentSequence < 0)
                modelInstance[2].currentSequence = modelInstance[2].sequences->sequenceCount;

            rlmSetAnimationInstanceSequence(&modelInstance[2], modelInstance[2].currentSequence);
        }

        if (IsKeyPressed(KEY_RIGHT))
        {
            modelInstance[2].currentSequence++;
            if (modelInstance[2].currentSequence >= modelInstance[2].sequences->sequenceCount)
                modelInstance[2].currentSequence = 0;

            rlmSetAnimationInstanceSequence(&modelInstance[2], modelInstance[2].currentSequence);
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            for (int i = 0; i < 5; i++)
                modelInstance[i].interpolate = !modelInstance[i].interpolate;
        }
    }
   
    return true;
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    BeginMode3D(ViewCam);

    for (int i = 0; i < 5; i++)
        rlmDrawModelWithPose(*modelInstance[i].model, modelInstance[i].transform, &modelInstance[i].currentPose);

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