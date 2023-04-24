// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundConcurrencyImporter.h"
#include "Sound/SoundConcurrency.h"

bool USoundConcurrencyImporter::ImportData() {
	try {
		USoundConcurrency* SoundConcurrency = NewObject<USoundConcurrency>(Package, USoundConcurrency::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties")->GetObjectField("Concurrency");

		FSoundConcurrencySettings SoundSettings;
		
		if (bool bLimitToOwner; Properties->TryGetBoolField("bLimitToOwner", bLimitToOwner))
			SoundSettings.bLimitToOwner = bLimitToOwner;
		if (bool bVolumeScaleCanRelease; Properties->TryGetBoolField("bVolumeScaleCanRelease", bVolumeScaleCanRelease))
			SoundSettings.bVolumeScaleCanRelease = bVolumeScaleCanRelease;
		if (int32 MaxCount; Properties->TryGetNumberField("MaxCount", MaxCount))
			SoundSettings.MaxCount = Properties->GetNumberField("MaxCount");
		if (FString ResolutionRule;Properties->TryGetStringField("ResolutionRule", ResolutionRule))
			SoundSettings.ResolutionRule = static_cast<EMaxConcurrentResolutionRule::Type>(StaticEnum<EMaxConcurrentResolutionRule::Type>()->GetValueByNameString(ResolutionRule));
		if (float RetriggerTime; Properties->TryGetNumberField("RetriggerTime", RetriggerTime))
			SoundSettings.RetriggerTime = Properties->GetNumberField("RetriggerTime");
		if (float VoiceStealReleaseTime; Properties->TryGetNumberField("VoiceStealReleaseTime", VoiceStealReleaseTime))
			SoundSettings.VoiceStealReleaseTime = Properties->GetNumberField("VoiceStealReleaseTime");
		if (float VolumeScaleAttackTime; Properties->TryGetNumberField("VolumeScaleAttackTime", VolumeScaleAttackTime))
			SoundSettings.VolumeScaleAttackTime = Properties->GetNumberField("VolumeScaleAttackTime");
		if (FString VolumeScaleMode; Properties->TryGetStringField("VolumeScaleMode", VolumeScaleMode)) 
			SoundSettings.VolumeScaleMode = static_cast<EConcurrencyVolumeScaleMode>(StaticEnum<EConcurrencyVolumeScaleMode>()->GetValueByNameString(VolumeScaleMode));
		if (float VolumeScaleReleaseTime; Properties->TryGetNumberField("VolumeScaleReleaseTime", VolumeScaleReleaseTime))
			SoundSettings.VolumeScaleReleaseTime = Properties->GetNumberField("VolumeScaleReleaseTime");

		SoundConcurrency->Concurrency = SoundSettings;

		// Handle edit changes, and add it to the content browser
		HandleAssetCreation(SoundConcurrency);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}