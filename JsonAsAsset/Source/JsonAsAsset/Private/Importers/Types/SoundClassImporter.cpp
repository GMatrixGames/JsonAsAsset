// Copyright JAA Contributors 2024-2025

#include "Importers/Types/SoundClassImporter.h"

#include "Sound/SoundClass.h"

bool USoundClassImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		TSharedPtr<FJsonObject> ChildClasses = JsonObject->GetObjectField("ChildClasses");
        
		USoundClass* SoundClass = NewObject<USoundClass>(Package, USoundClass::StaticClass(), *FileName, RF_Public | RF_Standalone);
        
		GetObjectSerializer()->DeserializeObjectProperties(Properties, SoundClass);
		GetObjectSerializer()->DeserializeObjectProperties(ChildClasses, SoundClass);
        
		SavePackage();
		HandleAssetCreation(SoundClass);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}