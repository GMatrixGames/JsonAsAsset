// Copyright Epic Games, Inc. All Rights Reserved.

#include "DerivedAssets/BlendSpaceDerived.h"

void UBlendSpaceDerived::SetAxisToScaleAnimationInput(const EBlendSpaceAxis AxisToScaleAnimationInput) {
	this->AxisToScaleAnimation = AxisToScaleAnimationInput;
}

void UBlendSpaceDerived::SetBlendParameterPrimary(const FBlendParameter& BlendParametersInput) {
	this->BlendParameters[0] = BlendParametersInput;
}

void UBlendSpaceDerived::SetBlendParameterSecondary(const FBlendParameter& BlendParametersInput) {
	this->BlendParameters[1] = BlendParametersInput;
}

void UBlendSpaceDerived::SetInterpolationParamPrimary(const FInterpolationParameter InterpolationParamInput) {
	this->InterpolationParam[0] = InterpolationParamInput;
}

void UBlendSpaceDerived::SetInterpolationParamSecondary(const FInterpolationParameter InterpolationParamInput) {
	this->InterpolationParam[1] = InterpolationParamInput;
}

void UBlendSpaceDerived::SetNotifyTriggerMode(const ENotifyTriggerMode::Type NotifyTriggerModeInput) {
	this->NotifyTriggerMode = NotifyTriggerModeInput;
}