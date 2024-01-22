// Copyright JAA Contributors 2023-2024

#include "Importers/SoundAttenuationImporter.h"
#include "Dom/JsonObject.h"
#include "Utilities/MathUtilities.h"

bool USoundAttenuationImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		USoundAttenuation* SoundAttenuation = NewObject<USoundAttenuation>(Package, USoundAttenuation::StaticClass(), *FileName, RF_Public | RF_Standalone);
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SoundAttenuation);

		// Handle edit changes, and add it to the content browser
		SavePackage();
		HandleAssetCreation(SoundAttenuation);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
