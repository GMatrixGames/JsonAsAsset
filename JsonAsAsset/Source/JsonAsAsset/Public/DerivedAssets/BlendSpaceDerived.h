// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/BlendSpace.h"

// We use this to set variables
// in the blend space asset
class UBlendSpaceDerived : public UBlendSpace {
public:
	void SetAxisToScaleAnimationInput(const EBlendSpaceAxis AxisToScaleAnimationInput);
	void SetBlendParameterPrimary(const FBlendParameter& BlendParametersInput);
	void SetBlendParameterSecondary(const FBlendParameter& BlendParametersInput);
	void SetInterpolationParamPrimary(const FInterpolationParameter InterpolationParamInput);
	void SetInterpolationParamSecondary(const FInterpolationParameter InterpolationParamInput);
	void SetNotifyTriggerMode(const ENotifyTriggerMode::Type NotifyTriggerModeInput);
};
