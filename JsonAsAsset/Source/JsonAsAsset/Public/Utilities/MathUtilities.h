// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/Color.h"

class FMathUtilities {
public:
	static FVector ObjectToVector(const FJsonObject* Object);
	static FVector ObjectToVector3f(const FJsonObject* Object);
	static FVector4 ObjectToVector4f(const FJsonObject* Object);
	static FRotator ObjectToRotator(const FJsonObject* Object);
	static FQuat ObjectToQuat(const FJsonObject* Object);
	static FLinearColor ObjectToLinearColor(const FJsonObject* Object);
	static FColor ObjectToColor(const FJsonObject* Object);
	static FLightingChannels ObjectToLightingChannels(const FJsonObject* Object);
	static FFloatInterval ObjectToFloatInterval(const FJsonObject* Object);
	static FRichCurveKey ObjectToRichCurveKey(const TSharedPtr<FJsonObject>& Object);
};
