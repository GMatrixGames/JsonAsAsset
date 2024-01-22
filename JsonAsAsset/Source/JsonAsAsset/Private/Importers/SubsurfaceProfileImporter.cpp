// Copyright JAA Contributors 2023-2024

#include "Importers/SubsurfaceProfileImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SubsurfaceProfile.h"
#include "Utilities/MathUtilities.h"

bool USubsurfaceProfileImporter::ImportData() {
	try {
		USubsurfaceProfile* SubsurfaceProfile = NewObject<USubsurfaceProfile>(Package, USubsurfaceProfile::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SubsurfaceProfile);

		// Handle edit changes, and add it to the content browser
		SavePackage();
		HandleAssetCreation(SubsurfaceProfile);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
