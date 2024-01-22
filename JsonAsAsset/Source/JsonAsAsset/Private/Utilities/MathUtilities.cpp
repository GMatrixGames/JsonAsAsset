// Copyright JAA Contributors 2023-2024

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

FLightingChannels FMathUtilities::ObjectToLightingChannels(const FJsonObject* Object) {
	FLightingChannels LightingChannels = FLightingChannels();

	LightingChannels.bChannel0 = Object->GetBoolField("bChannel0");
	LightingChannels.bChannel1 = Object->GetBoolField("bChannel1");
	LightingChannels.bChannel2 = Object->GetBoolField("bChannel2");

	return LightingChannels;
}

FFloatInterval FMathUtilities::ObjectToFloatInterval(const FJsonObject* Object) {
	return FFloatInterval(Object->GetNumberField("Min"), Object->GetNumberField("Max"));
}

FRichCurveKey FMathUtilities::ObjectToRichCurveKey(const TSharedPtr<FJsonObject>& Object) {
	FString InterpMode = Object->GetStringField("InterpMode");
	return FRichCurveKey(Object->GetNumberField("Time"), Object->GetNumberField("Value"), Object->GetNumberField("ArriveTangent"), Object->GetNumberField("LeaveTangent"), static_cast<ERichCurveInterpMode>(StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(InterpMode)));
}
