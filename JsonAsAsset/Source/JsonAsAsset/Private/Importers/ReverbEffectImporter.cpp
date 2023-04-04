// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/ReverbEffectImporter.h"
#include "Sound/ReverbEffect.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"

bool UReverbEffectImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		UReverbEffect* ReverbEffect = NewObject<UReverbEffect>(Package, UReverbEffect::StaticClass(), *FileName, RF_Public | RF_Standalone);

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(ReverbEffect)) return false;

		int64 AirAbsorptionGainHF;
		int64 DecayHFRatio;
		int64 DecayTime;
		int64 Density;
		int64 Diffusion;
		int64 Gain;
		int64 GainHF;
		int64 LateDelay;
		int64 LateGain;
		int64 ReflectionsDelay;
		int64 ReflectionsGain;
		bool bChanged;
		bool bBypassLateReflections;
		bool bBypassEarlyReflections;

		if (Properties->TryGetNumberField("AirAbsorptionGainHF", AirAbsorptionGainHF)) {
			ReverbEffect->AirAbsorptionGainHF = Properties->GetNumberField("AirAbsorptionGainHF");
		}
		if (Properties->TryGetNumberField("DecayHFRatio", DecayHFRatio)) {
			ReverbEffect->DecayHFRatio = Properties->GetNumberField("DecayHFRatio");
		}
		if (Properties->TryGetNumberField("DecayTime", DecayTime)) {
			ReverbEffect->DecayTime = Properties->GetNumberField("DecayTime");
		}
		if (Properties->TryGetNumberField("Density", Density)) {
			ReverbEffect->Density = Properties->GetNumberField("Density");
		}
		if (Properties->TryGetNumberField("Diffusion", Diffusion)) {
			ReverbEffect->Diffusion = Properties->GetNumberField("Diffusion");
		}
		if (Properties->TryGetNumberField("Gain", Gain)) {
			ReverbEffect->Gain = Properties->GetNumberField("Gain");
		}
		if (Properties->TryGetNumberField("GainHF", GainHF)) {
			ReverbEffect->GainHF = Properties->GetNumberField("GainHF");
		}
		if (Properties->TryGetNumberField("LateDelay", LateDelay)) {
			ReverbEffect->LateDelay = Properties->GetNumberField("LateDelay");
		}
		if (Properties->TryGetNumberField("LateGain", LateGain)) {
			ReverbEffect->LateGain = Properties->GetNumberField("LateGain");
		}
		if (Properties->TryGetNumberField("ReflectionsDelay", ReflectionsDelay)) {
			ReverbEffect->ReflectionsDelay = Properties->GetNumberField("ReflectionsDelay");
		}
		if (Properties->TryGetNumberField("ReflectionsGain", ReflectionsGain)) {
			ReverbEffect->ReflectionsGain = Properties->GetNumberField("ReflectionsGain");
		}
#if ENGINE_MAJOR_VERSION == 4
		int64 RoomRolloffFactor;

		if (Properties->TryGetNumberField("RoomRolloffFactor", RoomRolloffFactor)) {
			ReverbEffect->RoomRolloffFactor = Properties->GetNumberField("RoomRolloffFactor");
		}
#endif
		if (Properties->TryGetBoolField("bChanged", bChanged)) {
			ReverbEffect->bChanged = Properties->GetBoolField("bChanged");
		}
		if (Properties->TryGetBoolField("bBypassEarlyReflections", bBypassEarlyReflections)) {
			ReverbEffect->bBypassEarlyReflections = Properties->GetBoolField("bBypassEarlyReflections");
		}
		if (Properties->TryGetBoolField("bBypassLateReflections", bBypassLateReflections)) {
			ReverbEffect->bBypassLateReflections = Properties->GetBoolField("bBypassLateReflections");
		}
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
