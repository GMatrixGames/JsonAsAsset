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
		bool bEnableBurley;
		const TSharedPtr<FJsonObject>* BoundaryColorBleed;
		int64 ExtinctionScale;
		const TSharedPtr<FJsonObject>* FalloffColor;
		int64 IOR;
		int64 LobeMix;
		const TSharedPtr<FJsonObject>* MeanFreePathColor;
		int64 MeanFreePathDistance;
		int64 NormalScale;
		int64 Roughness0;
		int64 Roughness1;
		int64 ScatteringDistribution;
		int64 ScatterRadius;
		const TSharedPtr<FJsonObject>* SubsurfaceColor;
		const TSharedPtr<FJsonObject>* SurfaceAlbedo;
		const TSharedPtr<FJsonObject>* TransmissionTintColor;
		int64 WorldUnitScale;

		if (Settings->TryGetBoolField("bEnableBurley", bEnableBurley))
			Profile.bEnableBurley = bEnableBurley;

		if (Settings->TryGetObjectField("BoundaryColorBleed", BoundaryColorBleed))
			Profile.BoundaryColorBleed = FMathUtilities::ObjectToLinearColor(BoundaryColorBleed->Get());
		if (Settings->TryGetObjectField("MeanFreePathColor", MeanFreePathColor))
			Profile.MeanFreePathColor = FMathUtilities::ObjectToLinearColor(MeanFreePathColor->Get());
		if (Settings->TryGetObjectField("FalloffColor", FalloffColor))
			Profile.FalloffColor = FMathUtilities::ObjectToLinearColor(FalloffColor->Get());
		if (Settings->TryGetObjectField("TransmissionTintColor", TransmissionTintColor))
			Profile.TransmissionTintColor = FMathUtilities::ObjectToLinearColor(TransmissionTintColor->Get());
		if (Settings->TryGetObjectField("SubsurfaceColor", SubsurfaceColor))
			Profile.SubsurfaceColor = FMathUtilities::ObjectToLinearColor(SubsurfaceColor->Get());
		if (Settings->TryGetObjectField("SurfaceAlbedo", SurfaceAlbedo))
			Profile.SurfaceAlbedo = FMathUtilities::ObjectToLinearColor(SurfaceAlbedo->Get());

		if (Settings->TryGetNumberField("ExtinctionScale", ExtinctionScale))
			Profile.ExtinctionScale = Settings->GetNumberField("ExtinctionScale");
		if (Settings->TryGetNumberField("IOR", IOR))
			Profile.IOR = Settings->GetNumberField("IOR");
		if (Settings->TryGetNumberField("LobeMix", LobeMix))
			Profile.LobeMix = Settings->GetNumberField("LobeMix");
		if (Settings->TryGetNumberField("MeanFreePathDistance", MeanFreePathDistance))
			Profile.MeanFreePathDistance = Settings->GetNumberField("MeanFreePathDistance");
		if (Settings->TryGetNumberField("NormalScale", NormalScale))
			Profile.NormalScale = Settings->GetNumberField("NormalScale");
		if (Settings->TryGetNumberField("Roughness0", Roughness0))
			Profile.Roughness0 = Settings->GetNumberField("Roughness0");
		if (Settings->TryGetNumberField("Roughness1", Roughness1))
			Profile.Roughness1 = Settings->GetNumberField("Roughness1");
		if (Settings->TryGetNumberField("ScatteringDistribution", ScatteringDistribution))
			Profile.ScatteringDistribution = Settings->GetNumberField("ScatteringDistribution");
		if (Settings->TryGetNumberField("ScatterRadius", ScatterRadius))
			Profile.ScatterRadius = Settings->GetNumberField("ScatterRadius");
		if (Settings->TryGetNumberField("WorldUnitScale", WorldUnitScale))
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
