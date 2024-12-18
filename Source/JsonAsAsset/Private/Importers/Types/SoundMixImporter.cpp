// Copyright Epic Games, Inc. All Rights Reserved.


#include "Importers/Types/SoundMixImporter.h"
#include "Sound/SoundMix.h"

bool USoundMixImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		USoundMix* SoundMix = NewObject<USoundMix>(Package, USoundMix::StaticClass(), *FileName, RF_Public | RF_Standalone);
        
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SoundMix);
        
		SavePackage();
		HandleAssetCreation(SoundMix);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}