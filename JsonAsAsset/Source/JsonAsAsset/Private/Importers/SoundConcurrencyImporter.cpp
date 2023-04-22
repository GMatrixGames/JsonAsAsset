// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundConcurrencyImporter.h"

#include "Sound/SoundConcurrency.h"

bool USoundConcurrencyImporter::ImportData() {
	try {
		USoundConcurrency* SoundConcurrency = NewObject<USoundConcurrency>(Package, USoundConcurrency::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties")->GetObjectField("Concurrency");

		FSoundConcurrencySettings SoundSettings;
		bool bLimitToOwner;
		bool bVolumeScaleCanRelease;
		int32 MaxCount;
		FString ResolutionRule;
		float RetriggerTime;
		float VoiceStealReleaseTime;
		float VolumeScaleAttackTime;
		FString VolumeScaleMode;
		float VolumeScaleReleaseTime;


		if (Properties->TryGetBoolField("bLimitToOwner", bLimitToOwner))
			SoundSettings.bLimitToOwner = bLimitToOwner;
		if (Properties->TryGetBoolField("bVolumeScaleCanRelease", bVolumeScaleCanRelease))
			SoundSettings.bVolumeScaleCanRelease = bVolumeScaleCanRelease;
		if (Properties->TryGetNumberField("MaxCount", MaxCount))
			SoundSettings.MaxCount = Properties->GetNumberField("MaxCount");
		if (Properties->TryGetStringField("ResolutionRule", ResolutionRule))
			SoundSettings.ResolutionRule = static_cast<EMaxConcurrentResolutionRule::Type>(StaticEnum<EMaxConcurrentResolutionRule::Type>()->GetValueByNameString(ResolutionRule));
		if (Properties->TryGetNumberField("RetriggerTime", RetriggerTime))
			SoundSettings.RetriggerTime = Properties->GetNumberField("RetriggerTime");
		if (Properties->TryGetNumberField("VoiceStealReleaseTime", VoiceStealReleaseTime))
			SoundSettings.VoiceStealReleaseTime = Properties->GetNumberField("VoiceStealReleaseTime");
		if (Properties->TryGetNumberField("VolumeScaleAttackTime", VolumeScaleAttackTime))
			SoundSettings.VolumeScaleAttackTime = Properties->GetNumberField("VolumeScaleAttackTime");
		if (Properties->TryGetStringField("VolumeScaleMode", VolumeScaleMode)) 
			SoundSettings.VolumeScaleMode = static_cast<EConcurrencyVolumeScaleMode>(StaticEnum<EConcurrencyVolumeScaleMode>()->GetValueByNameString(VolumeScaleMode));
		if (Properties->TryGetNumberField("VolumeScaleReleaseTime", VolumeScaleReleaseTime))
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
