// Copyright JAA Contributors 2023-2024

#include "Importers/PhysicalMaterialImporter.h"

#include "Chaos/ChaosEngineInterface.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

bool UPhysicalMaterialImporter::ImportData() {
    try {
        UPhysicalMaterial* PhysicalMaterial = NewObject<UPhysicalMaterial>(Package, UPhysicalMaterial::StaticClass(), *FileName, RF_Public | RF_Standalone);
        TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
        GetObjectSerializer()->DeserializeObjectProperties(Properties, PhysicalMaterial);

        // Handle edit changes, and add it to the content browser
        SavePackage();
        if (!HandleAssetCreation(PhysicalMaterial)) return false;
    } catch (const char* Exception) {
        UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
        return false;
    }

    return true;
}