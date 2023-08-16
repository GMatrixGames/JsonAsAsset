// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SubsurfaceProfileImporter.h"

#include "Engine/SubsurfaceProfile.h"
#include "Utilities/MathUtilities.h"

bool USubsurfaceProfileImporter::ImportData() {
	try {
		USubsurfaceProfile* SubsurfaceProfile = NewObject<USubsurfaceProfile>(Cast<UObject>(Package), USubsurfaceProfile::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SubsurfaceProfile);

		// Handle edit changes, and add it to the content browser
		HandleAssetCreation(SubsurfaceProfile);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
