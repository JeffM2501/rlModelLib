#pragma once

#include "rlModels.h"

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif
	// todo, make this a simple convert function, there is no need to preserve the old model
	rlmModel rlmLoadFromModel(Model raylibModel);
	rlmModel rlmLoadFromModelEX(Model raylibModel, bool keepCPUData);

	// copies the skeleton out of a raylib model, the raylib model is not modified and must still be unloaded by the caller
	rlmSkeleton* rlmLoadSkeletonFromModel(Model raylibModel);
	// loads a model file only to get its skeleton, returns NULL if the file has no skeleton
	rlmSkeleton* rlmLoadSkeletonFromFile(const char* fileName);

	// raylib resamples glTF animations at 60 fps and M3D animations at ~60 fps (one frame every 17 ms)
	// IQM animations keep the file's own frame rate, which raylib does not expose, use the Ex loader with the right fps for those
	#define RLM_DEFAULT_ANIMATION_FPS 60.0f

	// animations whose bone count does not match the skeleton are loaded as empty sequences (keyframeCount 0)
	// sequences play at RLM_DEFAULT_ANIMATION_FPS
	rlmModelAniamtionSequence* rlmLoadModelAnimations(rlmSkeleton* skeleton, ModelAnimation* animations, int animationCount);
	rlmModelAniamtionSequence* rlmLoadModelAnimationsEx(rlmSkeleton* skeleton, ModelAnimation* animations, int animationCount, float fps);
#if defined(__cplusplus)
}
#endif