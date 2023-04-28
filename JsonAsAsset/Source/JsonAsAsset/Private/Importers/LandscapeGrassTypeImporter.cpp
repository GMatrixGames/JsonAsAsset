// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/LandscapeGrassTypeImporter.h"

#include "LandscapeGrassType.h"
#include "Utilities/MathUtilities.h"

bool ULandscapeGrassTypeImporter::ImportData() {
	try {
		ULandscapeGrassType* LandscapeGrassType = NewObject<ULandscapeGrassType>(Package, ULandscapeGrassType::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		if (bool bEnableDensityScaling; Properties->TryGetBoolField("bEnableDensityScaling", bEnableDensityScaling)) LandscapeGrassType->bEnableDensityScaling = Properties->GetBoolField("bEnableDensityScaling");

		if (const TArray<TSharedPtr<FJsonValue>>* GrassVarieties; Properties->TryGetArrayField("GrassVarieties", GrassVarieties)) {
			for (const TSharedPtr<FJsonValue> GrassVarientPtr : *GrassVarieties) {
				TSharedPtr<FJsonObject> GrassVar = GrassVarientPtr->AsObject();
				// FGrassVariety GrassVariety;
				//
				// if (bool AlignToSurface; GrassVar->TryGetBoolField("AlignToSurface", AlignToSurface)) GrassVariety.AlignToSurface = AlignToSurface;
				// if (bool bCastDynamicShadow; GrassVar->TryGetBoolField("bCastDynamicShadow", bCastDynamicShadow)) GrassVariety.bCastDynamicShadow = bCastDynamicShadow;
				// if (bool bKeepInstanceBufferCPUCopy; GrassVar->TryGetBoolField("bKeepInstanceBufferCPUCopy", bKeepInstanceBufferCPUCopy)) GrassVariety.bKeepInstanceBufferCPUCopy = bKeepInstanceBufferCPUCopy;
				// if (bool bReceivesDecals; GrassVar->TryGetBoolField("bReceivesDecals", bReceivesDecals)) GrassVariety.bReceivesDecals = bReceivesDecals;
				// if (bool bUseGrid; GrassVar->TryGetBoolField("bUseGrid", bUseGrid)) GrassVariety.bUseGrid = bUseGrid;
				// if (bool bUseLandscapeLightmap; GrassVar->TryGetBoolField("bUseLandscapeLightmap", bUseLandscapeLightmap)) GrassVariety.bUseLandscapeLightmap = bUseLandscapeLightmap;
				// if (int EndCullDistance; GrassVar->TryGetNumberField("EndCullDistance", EndCullDistance)) GrassVariety.EndCullDistance = FPerPlatformInt(EndCullDistance);
				// if (int StartCullDistance; GrassVar->TryGetNumberField("StartCullDistance", StartCullDistance)) GrassVariety.StartCullDistance = FPerPlatformInt(StartCullDistance);
				// if (float GrassDensity; GrassVar->TryGetNumberField("GrassDensity", GrassDensity)) GrassVariety.GrassDensity = FPerPlatformFloat(GrassDensity);
				// if (const TSharedPtr<FJsonObject>* GrassMesh; GrassVar->TryGetObjectField("GrassMesh", GrassMesh)) LoadObject(GrassMesh, GrassVariety.GrassMesh);
				// if (const TSharedPtr<FJsonObject>* LightingChannels; GrassVar->TryGetObjectField("LightingChannels", LightingChannels)) GrassVariety.LightingChannels = FMathUtilities::ObjectToLightingChannels(LightingChannels->Get());
				// if (int MinLOD; GrassVar->TryGetNumberField("MinLOD", MinLOD)) GrassVariety.MinLOD = MinLOD;
				// if (const TArray<TSharedPtr<FJsonValue>>* OverrideMaterials; GrassVar->TryGetArrayField("OverrideMaterials", OverrideMaterials)) LoadObject(GrassVar->GetArrayField("OverrideMaterials"), GrassVariety.OverrideMaterials);
				// if (float PlacementJitter; GrassVar->TryGetNumberField("PlacementJitter", PlacementJitter)) GrassVariety.PlacementJitter = PlacementJitter;
				// if (bool RandomRotation; GrassVar->TryGetBoolField("RandomRotation", RandomRotation)) GrassVariety.RandomRotation = RandomRotation;
				// if (FString Scaling; GrassVar->TryGetStringField("Scaling", Scaling)) GrassVariety.Scaling = static_cast<EGrassScaling>(StaticEnum<EGrassScaling>()->GetValueByNameString(Scaling));
				// if (const TSharedPtr<FJsonObject>* ScaleX; GrassVar->TryGetObjectField("ScaleX", ScaleX)) GrassVariety.ScaleX = FMathUtilities::ObjectToFloatInterval(ScaleX->Get());
				// if (const TSharedPtr<FJsonObject>* ScaleY; GrassVar->TryGetObjectField("ScaleY", ScaleY)) GrassVariety.ScaleY = FMathUtilities::ObjectToFloatInterval(ScaleY->Get());
				// if (const TSharedPtr<FJsonObject>* ScaleZ; GrassVar->TryGetObjectField("ScaleZ", ScaleZ)) GrassVariety.ScaleZ = FMathUtilities::ObjectToFloatInterval(ScaleZ->Get());
				//
				// LandscapeGrassType->GrassVarieties.Add(GrassVariety);
			}
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(LandscapeGrassType)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
