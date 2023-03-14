// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class FMathUtilities
{
public:
	static FVector ObjectToVector(const FJsonObject* Object);
	static FVector3f ObjectToVector3f(const FJsonObject* Object);
	static FRotator ObjectToRotator(const FJsonObject* Object);
	static FQuat ObjectToQuat(const FJsonObject* Object);
	static FLinearColor ObjectToLinearColor(const FJsonObject* Object);
};
