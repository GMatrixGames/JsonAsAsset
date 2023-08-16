// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/PhysicalMaterialImporter.h"

#include "UObject/ReflectedTypeAccessors.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

bool UPhysicalMaterialImporter::ImportData() {
    try {
        UPhysicalMaterial* PhysicalMaterial = NewObject<UPhysicalMaterial>(Package, UPhysicalMaterial::StaticClass(), *FileName, RF_Public | RF_Standalone);
        TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
        GetObjectSerializer()->DeserializeObjectProperties(Properties, PhysicalMaterial);

        // Handle edit changes, and add it to the content browser
        if (!HandleAssetCreation(PhysicalMaterial)) return false;
    } catch (const char* Exception) {
        UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
        return false;
    }

    return true;
}