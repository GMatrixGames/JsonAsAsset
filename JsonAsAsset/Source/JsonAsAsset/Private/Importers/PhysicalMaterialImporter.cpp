// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/PhysicalMaterialImporter.h"

#include "Chaos/ChaosEngineInterface.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

bool UPhysicalMaterialImporter::ImportData() {
    try {
        UPhysicalMaterial* PhysicalMaterial = NewObject<UPhysicalMaterial>(Package, UPhysicalMaterial::StaticClass(), *FileName, RF_Public | RF_Standalone);
        TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
        
        if (bool bOverrideFrictionCombineMode; Properties->TryGetBoolField("bOverrideFrictionCombineMode", bOverrideFrictionCombineMode))
            PhysicalMaterial->bOverrideFrictionCombineMode = Properties->GetBoolField("bOverrideFrictionCombineMode");
        if (bool bOverrideRestitutionCombineMode; Properties->TryGetBoolField("bOverrideRestitutionCombineMode", bOverrideRestitutionCombineMode))
            PhysicalMaterial->bOverrideRestitutionCombineMode = Properties->GetBoolField("bOverrideRestitutionCombineMode");
        if (float Density; Properties->TryGetNumberField("Density", Density))
            PhysicalMaterial->Density = Properties->GetNumberField("Density");
        // if (float DestructibleDamageThresholdScale; Properties->TryGetNumberField("DestructibleDamageThresholdScale", DestructibleDamageThresholdScale))
            // PhysicalMaterial->DestructibleDamageThresholdScale = Properties->GetNumberField("DestructibleDamageThresholdScale");
        if (float Friction; Properties->TryGetNumberField("Friction", Friction))
            PhysicalMaterial->Friction = Properties->GetNumberField("Friction");
        if (FString FrictionCombineMode; Properties->TryGetStringField("FrictionCombineMode", FrictionCombineMode))
            PhysicalMaterial->FrictionCombineMode = static_cast<EFrictionCombineMode::Type>(StaticEnum<EFrictionCombineMode::Type>()->GetValueByNameString(FrictionCombineMode));
        if (float RaiseMassToPower; Properties->TryGetNumberField("RaiseMassToPower", RaiseMassToPower))
            PhysicalMaterial->RaiseMassToPower = Properties->GetNumberField("RaiseMassToPower");
        if (float Restitution; Properties->TryGetNumberField("Restitution", Restitution))
            PhysicalMaterial->Restitution = Properties->GetNumberField("Restitution");
        if (FString RestitutionCombineMode; Properties->TryGetStringField("RestitutionCombineMode", RestitutionCombineMode))
            PhysicalMaterial->RestitutionCombineMode = static_cast<EFrictionCombineMode::Type>(StaticEnum<EFrictionCombineMode::Type>()->GetValueByNameString(RestitutionCombineMode));
        if (float SleepAngularVelocityThreshold; Properties->TryGetNumberField("SleepAngularVelocityThreshold", SleepAngularVelocityThreshold))
            PhysicalMaterial->SleepAngularVelocityThreshold = Properties->GetNumberField("SleepAngularVelocityThreshold");
        if (int32 SleepCounterThreshold; Properties->TryGetNumberField("SleepCounterThreshold", SleepCounterThreshold))
            PhysicalMaterial->SleepCounterThreshold = Properties->GetNumberField("SleepCounterThreshold");
        if (float SleepLinearVelocityThreshold; Properties->TryGetNumberField("SleepLinearVelocityThreshold", SleepLinearVelocityThreshold))
            PhysicalMaterial->SleepLinearVelocityThreshold = Properties->GetNumberField("SleepLinearVelocityThreshold");
        if (float StaticFriction; Properties->TryGetNumberField("StaticFriction", StaticFriction))
            PhysicalMaterial->StaticFriction = Properties->GetNumberField("StaticFriction");
        if (FString SurfaceType; Properties->TryGetStringField("SurfaceType", SurfaceType))
            PhysicalMaterial->SurfaceType = static_cast<EPhysicalSurface>(StaticEnum<EPhysicalSurface>()->GetValueByNameString(SurfaceType));

        // Handle edit changes, and add it to the content browser
        if (!HandleAssetCreation(PhysicalMaterial)) return false;
    } catch (const char* Exception) {
        UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
        return false;
    }

    return true;
}