// Copyright JAA Contributors 2024-2025

#include "Importers/Constructor/TemplatedImporter.h"

template <typename AssetType>
bool TemplatedImporter<AssetType>::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->SetPackageForDeserialization(Package);

		AssetType* Asset = NewObject<AssetType>(Package, AssetClass ? AssetClass : AssetType::StaticClass(), FName(FileName), RF_Public | RF_Standalone);
		GetObjectSerializer()->DeserializeObjectProperties(Properties, Asset);

		// USoundClass
		if (JsonObject->HasField("ChildClasses")) {
			TSharedPtr<FJsonObject> ChildClasses = JsonObject->GetObjectField("ChildClasses");
			GetObjectSerializer()->DeserializeObjectProperties(ChildClasses, Asset);
		}

		SavePackage();
		if (!HandleAssetCreation(Asset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
