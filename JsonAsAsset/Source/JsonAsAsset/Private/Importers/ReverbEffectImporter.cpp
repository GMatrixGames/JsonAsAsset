// Copyright JAA Contributors 2023-2024

#include "Importers/ReverbEffectImporter.h"
#include "Sound/ReverbEffect.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"

bool UReverbEffectImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		UReverbEffect* ReverbEffect = NewObject<UReverbEffect>(Package, UReverbEffect::StaticClass(), *FileName, RF_Public | RF_Standalone);
		GetObjectSerializer()->DeserializeObjectProperties(Properties, ReverbEffect);

		// Handle edit changes, and add it to the content browser
		SavePackage();
		if (!HandleAssetCreation(ReverbEffect)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
