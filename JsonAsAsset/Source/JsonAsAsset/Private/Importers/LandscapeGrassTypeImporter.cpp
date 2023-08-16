// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/LandscapeGrassTypeImporter.h"

#include "LandscapeGrassType.h"
#include "Utilities/MathUtilities.h"

bool ULandscapeGrassTypeImporter::ImportData() {
	try {
		ULandscapeGrassType* LandscapeGrassType = NewObject<ULandscapeGrassType>(Cast<UObject>(Package), ULandscapeGrassType::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->DeserializeObjectProperties(Properties, LandscapeGrassType);

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(LandscapeGrassType)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
