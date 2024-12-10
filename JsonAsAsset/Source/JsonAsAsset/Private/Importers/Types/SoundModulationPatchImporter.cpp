// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Types/SoundModulationPatchImporter.h"
#include "SoundModulationPatch.h"


bool USoundModulationPatchImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		USoundModulationPatch* SoundModulationPatch = NewObject<USoundModulationPatch>(Package, USoundModulationPatch::StaticClass(), *FileName, RF_Public | RF_Standalone);
        
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SoundModulationPatch);
        
		SavePackage();
		HandleAssetCreation(SoundModulationPatch);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
