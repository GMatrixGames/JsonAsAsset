// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/DataAssetImporter.h"
#include "Engine/DataAsset.h"

bool UDataAssetImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->SetPackageForDeserialization(Package);

		UPrimaryDataAsset* PrimaryDataAsset = NewObject<UPrimaryDataAsset>(Package, DataAssetClass, FName(FileName), RF_Public | RF_Standalone);
		GetObjectSerializer()->DeserializeObjectProperties(Properties, PrimaryDataAsset);
		
		if (!HandleAssetCreation(PrimaryDataAsset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
