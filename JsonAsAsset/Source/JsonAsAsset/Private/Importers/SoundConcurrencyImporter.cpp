// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundConcurrencyImporter.h"
#include "Sound/SoundConcurrency.h"

bool USoundConcurrencyImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		USoundConcurrency* SoundConcurrency = NewObject<USoundConcurrency>(Cast<UObject>(Package), USoundConcurrency::StaticClass(), *FileName, RF_Public | RF_Standalone);
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SoundConcurrency);

		// Handle edit changes, and add it to the content browser
		HandleAssetCreation(SoundConcurrency);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}