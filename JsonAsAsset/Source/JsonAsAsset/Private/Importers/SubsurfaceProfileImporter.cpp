// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SubsurfaceProfileImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SubsurfaceProfile.h"
#include "Utilities/MathUtilities.h"

bool USubsurfaceProfileImporter::ImportData() {
	try {
		USubsurfaceProfile* SubsurfaceProfile = NewObject<USubsurfaceProfile>(Package, USubsurfaceProfile::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Settings = JsonObject->GetObjectField("Properties")->GetObjectField("Settings");

		FSubsurfaceProfileStruct Profile;
		
		if (bool bEnableBurley; Settings->TryGetBoolField("bEnableBurley", bEnableBurley))
			Profile.bEnableBurley = bEnableBurley;

		if (const TSharedPtr<FJsonObject>* BoundaryColorBleed; Settings->TryGetObjectField("BoundaryColorBleed", BoundaryColorBleed))
			Profile.BoundaryColorBleed = FMathUtilities::ObjectToLinearColor(BoundaryColorBleed->Get());
		if (const TSharedPtr<FJsonObject>* MeanFreePathColor; Settings->TryGetObjectField("MeanFreePathColor", MeanFreePathColor))
			Profile.MeanFreePathColor = FMathUtilities::ObjectToLinearColor(MeanFreePathColor->Get());
		if (const TSharedPtr<FJsonObject>* FalloffColor; Settings->TryGetObjectField("FalloffColor", FalloffColor))
			Profile.FalloffColor = FMathUtilities::ObjectToLinearColor(FalloffColor->Get());
		if (const TSharedPtr<FJsonObject>* TransmissionTintColor; Settings->TryGetObjectField("TransmissionTintColor", TransmissionTintColor))
			Profile.TransmissionTintColor = FMathUtilities::ObjectToLinearColor(TransmissionTintColor->Get());
		if (const TSharedPtr<FJsonObject>* SubsurfaceColor; Settings->TryGetObjectField("SubsurfaceColor", SubsurfaceColor))
			Profile.SubsurfaceColor = FMathUtilities::ObjectToLinearColor(SubsurfaceColor->Get());
		if (const TSharedPtr<FJsonObject>* SurfaceAlbedo; Settings->TryGetObjectField("SurfaceAlbedo", SurfaceAlbedo))
			Profile.SurfaceAlbedo = FMathUtilities::ObjectToLinearColor(SurfaceAlbedo->Get());

		if (int64 ExtinctionScale; Settings->TryGetNumberField("ExtinctionScale", ExtinctionScale))
			Profile.ExtinctionScale = Settings->GetNumberField("ExtinctionScale");
		if (int64 IOR; Settings->TryGetNumberField("IOR", IOR))
			Profile.IOR = Settings->GetNumberField("IOR");
		if (int64 LobeMix; Settings->TryGetNumberField("LobeMix", LobeMix))
			Profile.LobeMix = Settings->GetNumberField("LobeMix");
		if (int64 MeanFreePathDistance; Settings->TryGetNumberField("MeanFreePathDistance", MeanFreePathDistance))
			Profile.MeanFreePathDistance = Settings->GetNumberField("MeanFreePathDistance");
		if (int64 NormalScale; Settings->TryGetNumberField("NormalScale", NormalScale))
			Profile.NormalScale = Settings->GetNumberField("NormalScale");
		if (int64 Roughness0; Settings->TryGetNumberField("Roughness0", Roughness0))
			Profile.Roughness0 = Settings->GetNumberField("Roughness0");
		if (int64 Roughness1 ;Settings->TryGetNumberField("Roughness1", Roughness1))
			Profile.Roughness1 = Settings->GetNumberField("Roughness1");
		if (int64 ScatteringDistribution; Settings->TryGetNumberField("ScatteringDistribution", ScatteringDistribution))
			Profile.ScatteringDistribution = Settings->GetNumberField("ScatteringDistribution");
		if (int64 ScatterRadius; Settings->TryGetNumberField("ScatterRadius", ScatterRadius))
			Profile.ScatterRadius = Settings->GetNumberField("ScatterRadius");
		if (int64 WorldUnitScale; Settings->TryGetNumberField("WorldUnitScale", WorldUnitScale))
			Profile.WorldUnitScale = Settings->GetNumberField("WorldUnitScale");

		SubsurfaceProfile->Settings = Profile;

		// Handle edit changes, and add it to the content browser
		HandleAssetCreation(SubsurfaceProfile);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
