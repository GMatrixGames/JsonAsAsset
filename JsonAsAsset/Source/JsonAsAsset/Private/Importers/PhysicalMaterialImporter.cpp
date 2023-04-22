// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/PhysicalMaterialImporter.h"

#include "Chaos/ChaosEngineInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

bool UPhysicalMaterialImporter::ImportData() {
    try {

        UPhysicalMaterial* PhysicalMaterial = NewObject<UPhysicalMaterial>(Package, UPhysicalMaterial::StaticClass(), *FileName, RF_Public | RF_Standalone);
        TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
        
        bool bOverrideFrictionCombineMode;
        bool bOverrideRestitutionCombineMode;
        float Density;
        float DestructibleDamageThresholdScale;
        float Friction;
        float RaiseMassToPower;
        float Restitution;
        float SleepAngularVelocityThreshold;
        int32 SleepCounterThreshold;
        float SleepLinearVelocityThreshold;
        float StaticFriction;
        FString SurfaceType;

        if (Properties->TryGetBoolField("bOverrideFrictionCombineMode", bOverrideFrictionCombineMode)) {
            PhysicalMaterial->bOverrideFrictionCombineMode = Properties->GetBoolField("bOverrideFrictionCombineMode");
        }
        if (Properties->TryGetBoolField("bOverrideRestitutionCombineMode", bOverrideRestitutionCombineMode)) {
            PhysicalMaterial->bOverrideRestitutionCombineMode = Properties->GetBoolField("bOverrideRestitutionCombineMode");
        }
        if (Properties->TryGetNumberField("Density", Density)) {
            PhysicalMaterial->Density = Properties->GetNumberField("Density");
        }
        if (Properties->TryGetNumberField("DestructibleDamageThresholdScale", DestructibleDamageThresholdScale)) {
            PhysicalMaterial->DestructibleDamageThresholdScale = Properties->GetNumberField("DestructibleDamageThresholdScale");
        }
        if (Properties->TryGetNumberField("Friction", Friction)) {
            PhysicalMaterial->Friction = Properties->GetNumberField("Friction");
        }
        if (Properties->TryGetNumberField("RaiseMassToPower", RaiseMassToPower)) {
            PhysicalMaterial->RaiseMassToPower = Properties->GetNumberField("RaiseMassToPower");
        }
        if (Properties->TryGetNumberField("Restitution", Restitution)) {
            PhysicalMaterial->Restitution = Properties->GetNumberField("Restitution");
        }
        if (Properties->TryGetNumberField("SleepAngularVelocityThreshold", SleepAngularVelocityThreshold)) {
            PhysicalMaterial->SleepAngularVelocityThreshold = Properties->GetNumberField("SleepAngularVelocityThreshold");
        }
        if (Properties->TryGetNumberField("SleepCounterThreshold", SleepCounterThreshold)) {
            PhysicalMaterial->SleepCounterThreshold = Properties->GetNumberField("SleepCounterThreshold");
        }
        if (Properties->TryGetNumberField("SleepLinearVelocityThreshold", SleepLinearVelocityThreshold)) {
            PhysicalMaterial->SleepLinearVelocityThreshold = Properties->GetNumberField("SleepLinearVelocityThreshold");
        }
        if (Properties->TryGetNumberField("StaticFriction", StaticFriction)) {
            PhysicalMaterial->StaticFriction = Properties->GetNumberField("StaticFriction");
        }
        if (Properties->TryGetStringField("SurfaceType", SurfaceType)) {
            PhysicalMaterial->SurfaceType = static_cast<EPhysicalSurface>(StaticEnum<EPhysicalSurface>()->GetValueByNameString(SurfaceType));
        }
        // Handle edit changes, and add it to the content browser
        if (!HandleAssetCreation(PhysicalMaterial)) return false;
        
    } catch (const char* Exception) {
        UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
        return false;
    }

    return true;
}