#pragma once

#include "rlModels.h"

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif
	// todo, make this a simple convert function, there is no need to preserve the old model
	rlmModel rlmLoadFromModel(Model raylibModel);
	rlmModel rlmLoadFromModelEX(Model raylibModel, bool keepCPUData);

	rlmModelAniamtionSequence* rlmLoadModelAnimations(rlmSkeleton* skeleton, ModelAnimation* animations, int animationCount);
#if defined(__cplusplus)
}
#endif