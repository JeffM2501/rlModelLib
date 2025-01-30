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

   newModel = rlmLoadFromModel(raylibModel);
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

   // DrawModel(raylibModel, Vector3Zeros, 1, WHITE);

    rlmDrawModel(newModel, rlmPQSIdentity());

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