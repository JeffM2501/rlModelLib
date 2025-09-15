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


void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);

    ViewCam.fovy = 45;
    ViewCam.position.z = -50;
    ViewCam.position.y = 10;
    ViewCam.target.y = 1;
    ViewCam.up.y = 1;

    newModel = rlmLoadModelGLTF("resources/robot.glb", true);
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

    return true;
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    BeginMode3D(ViewCam);


    DrawGrid(100, 1);

    rlmDrawModel(newModel, rlmPQSIdentity());

//     for (int i = 0; i < newModel.groupCount; i++)
//     {
//         for (int m = 0; m < newModel.groups[i].meshCount; m++)
//         {
//             for (int v = 0; v < newModel.groups[i].meshes[m].meshBuffers->vertexCount; v++)
//             {
//                 float* pVert = newModel.groups[i].meshes[m].meshBuffers->vertices + (v * 3);
//                 DrawCube(Vector3{ pVert[0], pVert[1], pVert[2] }, 0.1f, 0.1f, 0.1f, BLUE);
//             }
//        }
//     }

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