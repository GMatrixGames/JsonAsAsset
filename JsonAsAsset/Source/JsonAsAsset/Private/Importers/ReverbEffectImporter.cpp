// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/ReverbEffectImporter.h"
#include "Sound/ReverbEffect.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"

bool UReverbEffectImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		UReverbEffect* ReverbEffect = NewObject<UReverbEffect>(Package, UReverbEffect::StaticClass(), *FileName, RF_Public | RF_Standalone);
		
		if (int64 AirAbsorptionGainHF; Properties->TryGetNumberField("AirAbsorptionGainHF", AirAbsorptionGainHF))
			ReverbEffect->AirAbsorptionGainHF = Properties->GetNumberField("AirAbsorptionGainHF");
		if (int64 DecayHFRatio; Properties->TryGetNumberField("DecayHFRatio", DecayHFRatio))
			ReverbEffect->DecayHFRatio = Properties->GetNumberField("DecayHFRatio");
		if (int64 DecayTime; Properties->TryGetNumberField("DecayTime", DecayTime))
			ReverbEffect->DecayTime = Properties->GetNumberField("DecayTime");
		if (int64 Density; Properties->TryGetNumberField("Density", Density)) 
			ReverbEffect->Density = Properties->GetNumberField("Density");
		if (int64 Diffusion; Properties->TryGetNumberField("Diffusion", Diffusion)) 
			ReverbEffect->Diffusion = Properties->GetNumberField("Diffusion");
		if (int64 Gain; Properties->TryGetNumberField("Gain", Gain))
			ReverbEffect->Gain = Properties->GetNumberField("Gain");
		if (int64 GainHF; Properties->TryGetNumberField("GainHF", GainHF)) 
			ReverbEffect->GainHF = Properties->GetNumberField("GainHF");
		if (int64 LateDelay; Properties->TryGetNumberField("LateDelay", LateDelay)) 
			ReverbEffect->LateDelay = Properties->GetNumberField("LateDelay");
		if (int64 LateGain; Properties->TryGetNumberField("LateGain", LateGain)) 
			ReverbEffect->LateGain = Properties->GetNumberField("LateGain");
		if (int64 ReflectionsDelay; Properties->TryGetNumberField("ReflectionsDelay", ReflectionsDelay)) 
			ReverbEffect->ReflectionsDelay = Properties->GetNumberField("ReflectionsDelay");
		if (int64 ReflectionsGain;Properties->TryGetNumberField("ReflectionsGain", ReflectionsGain)) 
			ReverbEffect->ReflectionsGain = Properties->GetNumberField("ReflectionsGain");
		
#if ENGINE_MAJOR_VERSION == 4
		if (int64 RoomRolloffFactor; Properties->TryGetNumberField("RoomRolloffFactor", RoomRolloffFactor)) 
			ReverbEffect->RoomRolloffFactor = Properties->GetNumberField("RoomRolloffFactor");
#endif
		if (bool bChanged; Properties->TryGetBoolField("bChanged", bChanged)) 
			ReverbEffect->bChanged = Properties->GetBoolField("bChanged");
		if (bool bBypassEarlyReflections; Properties->TryGetBoolField("bBypassEarlyReflections", bBypassEarlyReflections)) 
			ReverbEffect->bBypassEarlyReflections = Properties->GetBoolField("bBypassEarlyReflections");
		if (bool bBypassLateReflections;Properties->TryGetBoolField("bBypassLateReflections", bBypassLateReflections)) 
			ReverbEffect->bBypassLateReflections = Properties->GetBoolField("bBypassLateReflections");

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(ReverbEffect)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
