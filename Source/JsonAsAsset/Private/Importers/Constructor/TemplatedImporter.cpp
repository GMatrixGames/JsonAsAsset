// Copyright JAA Contributors 2024-2025

#include "Importers/Constructor/TemplatedImporter.h"

template <typename AssetType>
bool TemplatedImporter<AssetType>::ImportData() {
	try {
		// Make Properties if it doesn't exist
		if (!JsonObject->HasField("Properties")) {
			JsonObject->SetObjectField("Properties", TSharedPtr<FJsonObject>());
		}
		
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->SetPackageForDeserialization(Package);

		AssetType* Asset = NewObject<AssetType>(Package, AssetClass ? AssetClass : AssetType::StaticClass(), FName(FileName), RF_Public | RF_Standalone);

		// Property MASH
		for (FString& PropertyName : PropertyMash) {
			if (JsonObject->HasField(PropertyName)) {
				TSharedPtr<FJsonValue> FieldValue = JsonObject->TryGetField(PropertyName);
				if (FieldValue.IsValid()) {
					Properties->SetField(PropertyName, FieldValue);
				}
			}
		}

		GetObjectSerializer()->DeserializeObjectProperties(Properties, Asset);

		return OnAssetCreation(Asset);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
	}

	return false;
}
