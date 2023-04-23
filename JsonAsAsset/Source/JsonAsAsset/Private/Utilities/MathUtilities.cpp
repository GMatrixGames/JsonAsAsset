// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/MathUtilities.h"
#include "Dom/JsonObject.h"

FVector FMathUtilities::ObjectToVector(const FJsonObject* Object) {
	return FVector(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"));
}

FVector3f FMathUtilities::ObjectToVector3f(const FJsonObject* Object) {
	return FVector3f(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"));
}

FVector4f FMathUtilities::ObjectToVector4f(const FJsonObject* Object) {
	return FVector4f(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"));
}

FRotator FMathUtilities::ObjectToRotator(const FJsonObject* Object) {
	return FRotator(Object->GetNumberField("Pitch"), Object->GetNumberField("Yaw"), Object->GetNumberField("Roll"));
}

FQuat FMathUtilities::ObjectToQuat(const FJsonObject* Object) {
	return FQuat(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"), Object->GetNumberField("W"));
}

FLinearColor FMathUtilities::ObjectToLinearColor(const FJsonObject* Object) {
	return FLinearColor(Object->GetNumberField("R"), Object->GetNumberField("G"), Object->GetNumberField("B"), Object->GetNumberField("A"));
}

FColor FMathUtilities::ObjectToColor(const FJsonObject* Object) {
	return ObjectToLinearColor(Object).ToFColor(true);
}
