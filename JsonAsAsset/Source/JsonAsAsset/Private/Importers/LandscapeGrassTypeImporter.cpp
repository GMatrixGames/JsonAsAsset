// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/LandscapeGrassTypeImporter.h"

#include "Chaos/ChaosEngineInterface.h"
#include "Utilities/MathUtilities.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "LandscapeGrassType.h"

bool ULandscapeGrassTypeImporter::ImportData() {
    try {
        ULandscapeGrassType* LandscapeGrassType = NewObject<ULandscapeGrassType>(Package, ULandscapeGrassType::StaticClass(), *FileName, RF_Public | RF_Standalone);
        TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
        
        if (bool bEnableDensityScaling; Properties->TryGetBoolField("bEnableDensityScaling", bEnableDensityScaling))
            LandscapeGrassType->bEnableDensityScaling = Properties->GetBoolField("bEnableDensityScaling");

        if (const TArray<TSharedPtr<FJsonValue>>* GrassVarieties; Properties->TryGetArrayField("GrassVarieties", GrassVarieties))
            for (const TSharedPtr<FJsonValue> GrassVarientPtr : *GrassVarieties) {
                TSharedPtr<FJsonObject> GrassVar = GrassVarientPtr->AsObject();
                FGrassVariety GrassVarient = FGrassVariety();

                if (bool AlignToSurface; GrassVar->TryGetBoolField("AlignToSurface", AlignToSurface)) GrassVarient.AlignToSurface = AlignToSurface;
                if (bool bCastDynamicShadow; GrassVar->TryGetBoolField("bCastDynamicShadow", bCastDynamicShadow)) GrassVarient.bCastDynamicShadow = bCastDynamicShadow;
                if (bool bKeepInstanceBufferCPUCopy; GrassVar->TryGetBoolField("bKeepInstanceBufferCPUCopy", bKeepInstanceBufferCPUCopy)) GrassVarient.bKeepInstanceBufferCPUCopy = bKeepInstanceBufferCPUCopy;
                if (bool bReceivesDecals; GrassVar->TryGetBoolField("bReceivesDecals", bReceivesDecals)) GrassVarient.bReceivesDecals = bReceivesDecals;
                if (bool bUseGrid; GrassVar->TryGetBoolField("bUseGrid", bUseGrid)) GrassVarient.bUseGrid = bUseGrid;
                if (bool bUseLandscapeLightmap; GrassVar->TryGetBoolField("bUseLandscapeLightmap", bUseLandscapeLightmap)) GrassVarient.bUseLandscapeLightmap = bUseLandscapeLightmap;

                if (int32 EndCullDistance; GrassVar->TryGetNumberField("EndCullDistance", EndCullDistance)) GrassVarient.EndCullDistance = FPerPlatformInt(EndCullDistance);
                if (int32 StartCullDistance; GrassVar->TryGetNumberField("StartCullDistance", StartCullDistance)) GrassVarient.StartCullDistance = FPerPlatformInt(StartCullDistance);
                if (float GrassDensity; GrassVar->TryGetNumberField("GrassDensity", GrassDensity)) GrassVarient.GrassDensity = FPerPlatformFloat(GrassDensity);
                
                if (const TSharedPtr<FJsonObject>* GrassMesh; GrassVar->TryGetObjectField("GrassMesh", GrassMesh))
                    LoadObject(GrassMesh, GrassVarient.GrassMesh);

                if (const TSharedPtr<FJsonObject>* LightingChannels; GrassVar->TryGetObjectField("LightingChannels", LightingChannels))
                    GrassVarient.LightingChannels = FMathUtilities::ObjectToLightingChannels(LightingChannels->Get());

                if (int32 MinLOD; GrassVar->TryGetNumberField("MinLOD", MinLOD)) GrassVarient.MinLOD = MinLOD;
                if (const TArray<TSharedPtr<FJsonValue>>* OverrideMaterials; GrassVar->TryGetArrayField("OverrideMaterials", OverrideMaterials))
                    LoadObject(GrassVar->GetArrayField("OverrideMaterials"), GrassVarient.OverrideMaterials);

                if (float PlacementJitter; GrassVar->TryGetNumberField("PlacementJitter", PlacementJitter)) GrassVarient.PlacementJitter = PlacementJitter;
                if (bool RandomRotation; GrassVar->TryGetBoolField("RandomRotation", RandomRotation)) GrassVarient.RandomRotation = RandomRotation;
                
                if (FString Scaling; GrassVar->TryGetStringField("Scaling", Scaling)) GrassVarient.Scaling = static_cast<EGrassScaling>(StaticEnum<EGrassScaling>()->GetValueByNameString(Scaling));

                if (const TSharedPtr<FJsonObject>* ScaleX; GrassVar->TryGetObjectField("ScaleX", ScaleX))
                    GrassVarient.ScaleX = FMathUtilities::ObjectToFloatInterval(ScaleX->Get());

                if (const TSharedPtr<FJsonObject>* ScaleY; GrassVar->TryGetObjectField("ScaleY", ScaleY))
                    GrassVarient.ScaleY = FMathUtilities::ObjectToFloatInterval(ScaleY->Get());

                if (const TSharedPtr<FJsonObject>* ScaleZ; GrassVar->TryGetObjectField("ScaleZ", ScaleZ))
                    GrassVarient.ScaleZ = FMathUtilities::ObjectToFloatInterval(ScaleZ->Get());

                LandscapeGrassType->GrassVarieties.Add(GrassVarient);
            }

        // Handle edit changes, and add it to the content browser
        if (!HandleAssetCreation(LandscapeGrassType)) return false;
    } catch (const char* Exception) {
        UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
        return false;
    }

    return true;
}