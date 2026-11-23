/*
Raylib example file.
Retargeting example: one walk animation, authored on the walk.glb character, played on two
differently proportioned characters (characterSmall.glb and characterLargeFemale.glb).

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

#include <string.h>

struct Character
{
    const char* label = nullptr;
    rlmModel model = { 0 };

    rlmRetargetMap map = { 0 };                 // only used by the targets
    rlmModelAnimationSet retargeted = { 0 };    // the source animation baked onto this character

    rlmAnimatedModelInstance instance = { 0 };
};

Camera3D ViewCam = { 0 };

Texture2D SkinTexture = { 0 };
Shader SkinningShader = { 0 };

Character Source;           // walk.glb, the character the animation was made for
Character Targets[2];       // characterSmall.glb and characterLargeFemale.glb
Character* AllCharacters[] = { &Source, &Targets[0], &Targets[1] };
rlmModelAnimationSet SourceAnimations = { 0 };
int WalkSequence = 0;

bool UseRetargeted = true;
bool ShowSource = true;

rlmModel LoadCharacterModel(const char* fileName)
{
    Model raylibModel = LoadModel(fileName);

    // none of the character files embed a texture, they all share one skin
    for (int i = 0; i < raylibModel.materialCount; i++)
        raylibModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = SkinTexture;

    rlmModel model = rlmLoadFromModel(raylibModel);

    for (int i = 0; i < model.groupCount; i++)
    {
        model.groups[i].material.baseChannel.ownsTexture = false; // shared, unloaded once in GameCleanup
        rlmSetMaterialDefShader(&model.groups[i].material, SkinningShader);
    }

    return model;
}

void StartInstance(Character& character, rlmModelAnimationSet* animations, float x)
{
    character.instance.model = &character.model;
    character.instance.sequences = animations;
    character.instance.interpolate = true;
    character.instance.currentPose = rlmLoadPoseFromModel(character.model);
    character.instance.transform = rlmPQSTranslation(x, 0, 0);

    rlmSetAnimationInstanceSequence(&character.instance, WalkSequence);
}

// retargeting the source's own reference pose must reproduce the target's reference pose exactly
float ReferencePoseError(const rlmRetargetMap& map)
{
    rlmAnimationKeyframe frame = rlmRetargetKeyframe(&map, map.source->retargetInfo->referencePose);

    float worst = 0;
    for (int i = 0; i < map.target->boneCount; i++)
    {
        const rlmPQSTransorm& expected = map.target->retargetInfo->referencePose.boneTransforms[i];
        worst = fmaxf(worst, Vector3Distance(frame.boneTransforms[i].position, expected.position));

        Quaternion a = frame.boneTransforms[i].rotation;
        Quaternion b = expected.rotation;
        worst = fmaxf(worst, 1.0f - fminf(fabsf(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w), 1.0f));
    }

    rlmUnloadAnimationKeyframe(&frame);
    return worst;
}

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "Retargeting Example");
    SetTargetFPS(144);

    ViewCam.fovy = 45;
    ViewCam.position = Vector3{ 0, 2.2f, 7.5f };
    ViewCam.target = Vector3{ 0, 1.2f, 0 };
    ViewCam.up = Vector3{ 0, 1, 0 };

    SkinTexture = LoadTexture("resources/character_skin.png");
    SkinningShader = LoadShader("resources/skinning.vs", "resources/skinning.fs");

    // the source: the walk animation and the skeleton it was authored on both come from walk.glb
    Source.label = "walk.glb (source)";
    Source.model = LoadCharacterModel("resources/walk.glb");

    int animationCount = 0;
    ModelAnimation* animations = LoadModelAnimations("resources/walk.glb", &animationCount);
    SourceAnimations.sequenceCount = animationCount;
    SourceAnimations.sequences = rlmLoadModelAnimations(Source.model.skeleton, animations, animationCount);

    for (int i = 0; i < SourceAnimations.sequenceCount; i++)
    {
        if (strcmp(SourceAnimations.sequences[i].name, "Walk") == 0)
            WalkSequence = i;
    }

    const rlmModelAniamtionSequence& walk = SourceAnimations.sequences[WalkSequence];
    TraceLog(LOG_INFO, "retarget_example : %s has %d keyframes at %.0f fps, %.3f seconds per loop", walk.name, walk.keyframeCount, walk.fps, walk.keyframeCount / walk.fps);

    // the targets: same bone names, different proportions
    Targets[0].label = "characterSmall.glb";
    Targets[0].model = LoadCharacterModel("resources/characterSmall.glb");

    Targets[1].label = "characterLargeFemale.glb";
    Targets[1].model = LoadCharacterModel("resources/characterLargeFemale.glb");

    for (Character& target : Targets)
    {
        // match bones by name, then bake every source sequence onto the target skeleton once at load time
        target.map = rlmLoadRetargetMap(Source.model.skeleton, target.model.skeleton);
        target.retargeted = rlmRetargetAnimationSet(&target.map, &SourceAnimations);

        TraceLog(LOG_INFO, "retarget_example : %s mapped %d of %d bones, hip scale %f, reference pose error %f",
            target.label, target.map.mappedCount, target.model.skeleton->boneCount, target.map.translationScale, ReferencePoseError(target.map));
    }

    StartInstance(Targets[0], &Targets[0].retargeted, -2.0f);
    StartInstance(Source, &SourceAnimations, 0.0f);
    StartInstance(Targets[1], &Targets[1].retargeted, 2.0f);
}

void GameCleanup()
{
    for (Character& target : Targets)
    {
        rlmUnloadAnimationSet(&target.retargeted);
        rlmUnloadRetargetMap(&target.map);
    }

    for (Character* character : AllCharacters)
    {
        rlmUnloadPose(&character->instance.currentPose);
        rlmUnloadModel(&character->model);
    }

    rlmUnloadAnimationSet(&SourceAnimations);

    UnloadTexture(SkinTexture);
    UnloadShader(SkinningShader);
    CloseWindow();
}

void SetUseRetargeted(bool useRetargeted)
{
    UseRetargeted = useRetargeted;

    // without retargeting the targets play the source keyframes directly, this only works at all because
    // the skeletons have the same bone order, and it pulls every bone to the source character's proportions
    for (Character& target : Targets)
    {
        int frame = target.instance.currentFrame;
        target.instance.sequences = UseRetargeted ? &target.retargeted : &SourceAnimations;
        rlmSetAnimationInstanceSequence(&target.instance, WalkSequence);
        target.instance.currentFrame = frame;
    }
}

bool GameUpdate()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        UpdateCamera(&ViewCam, CAMERA_THIRD_PERSON);

    if (IsKeyPressed(KEY_R))
        SetUseRetargeted(!UseRetargeted);

    if (IsKeyPressed(KEY_S))
        ShowSource = !ShowSource;

    if (IsKeyPressed(KEY_SPACE))
    {
        for (Character* character : AllCharacters)
            character->instance.interpolate = !character->instance.interpolate;
    }

    for (Character* character : AllCharacters)
        rlmAdvanceAnimationInstance(&character->instance, GetFrameTime());

    return true;
}

void DrawCharacter(const Character& character)
{
    rlmDrawModelWithPose(character.model, character.instance.transform, (rlmModelAnimationPose*)&character.instance.currentPose);
}

void DrawLabel(const Character& character, Color color)
{
    Vector2 screen = GetWorldToScreen(Vector3Add(character.instance.transform.position, Vector3{ 0, -0.25f, 0 }), ViewCam);
    int width = MeasureText(character.label, 20);
    DrawText(character.label, (int)screen.x - width / 2, (int)screen.y, 20, color);
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode3D(ViewCam);
    DrawGrid(20, 0.5f);

    for (const Character& target : Targets)
        DrawCharacter(target);

    if (ShowSource)
        DrawCharacter(Source);
    EndMode3D();

    for (const Character& target : Targets)
        DrawLabel(target, WHITE);
    if (ShowSource)
        DrawLabel(Source, SKYBLUE);

    DrawText(UseRetargeted ? "Targets: RETARGETED walk" : "Targets: RAW source keyframes (no retargeting)", 10, 10, 20, UseRetargeted ? GREEN : ORANGE);
    DrawText("R: toggle retargeting   S: toggle source   SPACE: toggle interpolation   Right mouse: orbit", 10, 36, 20, LIGHTGRAY);

    EndDrawing();
}

int main(int argc, char** argv)
{
    // --raw starts with retargeting off, --screenshot <file> saves a frame after a second and exits (for automated checks)
    const char* screenshotFile = nullptr;
    bool startRaw = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--raw") == 0)
            startRaw = true;
        else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc)
            screenshotFile = argv[++i];
    }

    GameInit();
    if (startRaw)
        SetUseRetargeted(false);

    int frameCount = 0;
    while (!WindowShouldClose())
    {
        if (!GameUpdate())
            break;

        GameDraw();

        if (screenshotFile && ++frameCount == 60)
        {
            TakeScreenshot(screenshotFile);
            break;
        }
    }
    GameCleanup();

    return 0;
}
