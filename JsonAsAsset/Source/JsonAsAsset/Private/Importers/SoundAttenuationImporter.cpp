// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundAttenuationImporter.h"
#include "Dom/JsonObject.h"
#include "Utilities/MathUtilities.h"

bool USoundAttenuationImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		USoundAttenuation* SoundAttenuation = NewObject<USoundAttenuation>(Package, USoundAttenuation::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Attenuation = Properties->GetObjectField("Attenuation");

		// booleans
		if (bool bApplyNormalizationToStereoSounds; Attenuation->TryGetBoolField("bApplyNormalizationToStereoSounds", bApplyNormalizationToStereoSounds))
			SoundAttenuation->Attenuation.bApplyNormalizationToStereoSounds = Attenuation->GetBoolField("bApplyNormalizationToStereoSounds");
		if (bool bAttenuate; Attenuation->TryGetBoolField("bAttenuate", bAttenuate))
			SoundAttenuation->Attenuation.bAttenuate = Attenuation->GetBoolField("bAttenuate");
		if (bool bAttenuateWithLPF; Attenuation->TryGetBoolField("bAttenuateWithLPF", bAttenuateWithLPF))
			SoundAttenuation->Attenuation.bAttenuateWithLPF = Attenuation->GetBoolField("bAttenuateWithLPF");
		if (bool bEnableFocusInterpolation; Attenuation->TryGetBoolField("bEnableFocusInterpolation", bEnableFocusInterpolation))
			SoundAttenuation->Attenuation.bEnableFocusInterpolation = Attenuation->GetBoolField("bEnableFocusInterpolation");
		if (bool bEnableListenerFocus; Attenuation->TryGetBoolField("bEnableListenerFocus", bEnableListenerFocus))
			SoundAttenuation->Attenuation.bEnableListenerFocus = Attenuation->GetBoolField("bEnableListenerFocus");
		if (bool bEnableLogFrequencyScaling; Attenuation->TryGetBoolField("bEnableLogFrequencyScaling", bEnableLogFrequencyScaling))
			SoundAttenuation->Attenuation.bEnableLogFrequencyScaling = Attenuation->GetBoolField("bEnableLogFrequencyScaling");
		if (bool bEnableOcclusion; Attenuation->TryGetBoolField("bEnableOcclusion", bEnableOcclusion))
			SoundAttenuation->Attenuation.bEnableOcclusion = Attenuation->GetBoolField("bEnableOcclusion");
		if (bool bEnablePriorityAttenuation; Attenuation->TryGetBoolField("bEnablePriorityAttenuation", bEnablePriorityAttenuation))
			SoundAttenuation->Attenuation.bEnablePriorityAttenuation = Attenuation->GetBoolField("bEnablePriorityAttenuation");
		if (bool bEnableReverbSend; Attenuation->TryGetBoolField("bEnableReverbSend", bEnableReverbSend))
			SoundAttenuation->Attenuation.bEnableReverbSend = Attenuation->GetBoolField("bEnableReverbSend");
		if (bool bEnableSubmixSends; Attenuation->TryGetBoolField("bEnableSubmixSends", bEnableSubmixSends))
			SoundAttenuation->Attenuation.bEnableSubmixSends = Attenuation->GetBoolField("bEnableSubmixSends");
		if (bool bSpatialize; Attenuation->TryGetBoolField("bSpatialize", bSpatialize))
			SoundAttenuation->Attenuation.bSpatialize = Attenuation->GetBoolField("bSpatialize");
		if (bool bUseComplexCollisionForOcclusion; Attenuation->TryGetBoolField("bUseComplexCollisionForOcclusion", bUseComplexCollisionForOcclusion))
			SoundAttenuation->Attenuation.bUseComplexCollisionForOcclusion = Attenuation->GetBoolField("bUseComplexCollisionForOcclusion");
		
		//ints
		if (int64 ConeOffset; Attenuation->TryGetNumberField("ConeOffset", ConeOffset))
			SoundAttenuation->Attenuation.ConeOffset = Attenuation->GetNumberField("ConeOffset");
		if (int64 dBAttenuationAtMax; Attenuation->TryGetNumberField("dBAttenuationAtMax", dBAttenuationAtMax))
			SoundAttenuation->Attenuation.dBAttenuationAtMax = Attenuation->GetNumberField("dBAttenuationAtMax");
		if (int64 FalloffDistance; Attenuation->TryGetNumberField("FalloffDistance", FalloffDistance))
			SoundAttenuation->Attenuation.FalloffDistance = Attenuation->GetNumberField("FalloffDistance");
		if (int64 BinauralRadius; Attenuation->TryGetNumberField("BinauralRadius", BinauralRadius))
			SoundAttenuation->Attenuation.BinauralRadius = Attenuation->GetNumberField("BinauralRadius");
		if (int64 FocusAttackInterpSpeed;Attenuation->TryGetNumberField("FocusAttackInterpSpeed", FocusAttackInterpSpeed)) 
			SoundAttenuation->Attenuation.FocusAttackInterpSpeed = Attenuation->GetNumberField("FocusAttackInterpSpeed");
		if (int64 FocusAzimuth; Attenuation->TryGetNumberField("FocusAzimuth", FocusAzimuth))
			SoundAttenuation->Attenuation.FocusAzimuth = Attenuation->GetNumberField("FocusAzimuth");
		if (int64 FocusDistanceScale; Attenuation->TryGetNumberField("FocusDistanceScale", FocusDistanceScale))
			SoundAttenuation->Attenuation.FocusDistanceScale = Attenuation->GetNumberField("FocusDistanceScale");
		if (int64 FocusPriorityScale; Attenuation->TryGetNumberField("FocusPriorityScale", FocusPriorityScale)) 
			SoundAttenuation->Attenuation.FocusPriorityScale = Attenuation->GetNumberField("FocusPriorityScale");
		if (int64 FocusReleaseInterpSpeed; Attenuation->TryGetNumberField("FocusReleaseInterpSpeed", FocusReleaseInterpSpeed)) 
			SoundAttenuation->Attenuation.FocusReleaseInterpSpeed = Attenuation->GetNumberField("FocusReleaseInterpSpeed");
		if (int64 FocusVolumeAttenuation; Attenuation->TryGetNumberField("FocusVolumeAttenuation", FocusVolumeAttenuation)) 
			SoundAttenuation->Attenuation.FocusVolumeAttenuation = Attenuation->GetNumberField("FocusVolumeAttenuation");
		if (int64 HPFFrequencyAtMax; Attenuation->TryGetNumberField("HPFFrequencyAtMax", HPFFrequencyAtMax)) 
			SoundAttenuation->Attenuation.HPFFrequencyAtMax = Attenuation->GetNumberField("HPFFrequencyAtMax");
		if (int64 HPFFrequencyAtMin; Attenuation->TryGetNumberField("HPFFrequencyAtMin", HPFFrequencyAtMin)) 
			SoundAttenuation->Attenuation.HPFFrequencyAtMin = Attenuation->GetNumberField("HPFFrequencyAtMin");
		if (int64 ManualPriorityAttenuation; Attenuation->TryGetNumberField("ManualPriorityAttenuation", ManualPriorityAttenuation)) 
			SoundAttenuation->Attenuation.ManualPriorityAttenuation = Attenuation->GetNumberField("ManualPriorityAttenuation");
		if (int64 LPFRadiusMin; Attenuation->TryGetNumberField("LPFRadiusMin", LPFRadiusMin)) 
			SoundAttenuation->Attenuation.LPFRadiusMin = Attenuation->GetNumberField("LPFRadiusMin");
		if (int64 LPFRadiusMax; Attenuation->TryGetNumberField("LPFRadiusMax", LPFRadiusMax)) 
			SoundAttenuation->Attenuation.LPFRadiusMax = Attenuation->GetNumberField("LPFRadiusMax");
		if (int64 ManualReverbSendLevel; Attenuation->TryGetNumberField("ManualReverbSendLevel", ManualReverbSendLevel)) 
			SoundAttenuation->Attenuation.ManualReverbSendLevel = Attenuation->GetNumberField("ManualReverbSendLevel");
		if (int64 NonFocusAzimuth; Attenuation->TryGetNumberField("NonFocusAzimuth", NonFocusAzimuth)) 
			SoundAttenuation->Attenuation.NonFocusAzimuth = Attenuation->GetNumberField("NonFocusAzimuth");
		if (int64 NonFocusDistanceScale; Attenuation->TryGetNumberField("NonFocusDistanceScale", NonFocusDistanceScale)) 
			SoundAttenuation->Attenuation.NonFocusDistanceScale = Attenuation->GetNumberField("NonFocusDistanceScale");
		if (int64 NonFocusPriorityScale; Attenuation->TryGetNumberField("NonFocusPriorityScale", NonFocusPriorityScale)) 
			SoundAttenuation->Attenuation.NonFocusPriorityScale = Attenuation->GetNumberField("NonFocusPriorityScale");
		if (int64 NonFocusVolumeAttenuation; Attenuation->TryGetNumberField("NonFocusVolumeAttenuation", NonFocusVolumeAttenuation)) 
			SoundAttenuation->Attenuation.NonFocusVolumeAttenuation = Attenuation->GetNumberField("NonFocusVolumeAttenuation");
		if (int64 OcclusionInterpolationTime; Attenuation->TryGetNumberField("OcclusionInterpolationTime", OcclusionInterpolationTime)) 
			SoundAttenuation->Attenuation.OcclusionInterpolationTime = Attenuation->GetNumberField("OcclusionInterpolationTime");
		if (int64 OcclusionLowPassFilterFrequency; Attenuation->TryGetNumberField("OcclusionLowPassFilterFrequency", OcclusionLowPassFilterFrequency)) 
			SoundAttenuation->Attenuation.OcclusionLowPassFilterFrequency = Attenuation->GetNumberField("OcclusionLowPassFilterFrequency");
		if (int64 OcclusionVolumeAttenuation; Attenuation->TryGetNumberField("OcclusionVolumeAttenuation", OcclusionVolumeAttenuation)) 
			SoundAttenuation->Attenuation.OcclusionVolumeAttenuation = Attenuation->GetNumberField("OcclusionVolumeAttenuation");
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3) || ENGINE_MAJOR_VERSION == 4

		if (int64 OmniRadius; Attenuation->TryGetNumberField("OmniRadius", OmniRadius)) 
			SoundAttenuation->Attenuation.OmniRadius = Attenuation->GetNumberField("OmniRadius");
#endif
		if (int64 PriorityAttenuationDistanceMax; Attenuation->TryGetNumberField("PriorityAttenuationDistanceMax", PriorityAttenuationDistanceMax)) 
			SoundAttenuation->Attenuation.PriorityAttenuationDistanceMax = Attenuation->GetNumberField("PriorityAttenuationDistanceMax");
		if (int64 PriorityAttenuationDistanceMin; Attenuation->TryGetNumberField("PriorityAttenuationDistanceMin", PriorityAttenuationDistanceMin)) 
			SoundAttenuation->Attenuation.PriorityAttenuationDistanceMin = Attenuation->GetNumberField("PriorityAttenuationDistanceMin");
		if (int64 PriorityAttenuationMax; Attenuation->TryGetNumberField("PriorityAttenuationMax", PriorityAttenuationMax)) 
			SoundAttenuation->Attenuation.PriorityAttenuationMax = Attenuation->GetNumberField("PriorityAttenuationMax");
		if (int64 PriorityAttenuationMin; Attenuation->TryGetNumberField("PriorityAttenuationMin", PriorityAttenuationMin)) 
			SoundAttenuation->Attenuation.PriorityAttenuationMin = Attenuation->GetNumberField("PriorityAttenuationMin");
		if (int64 ReverbDistanceMax; Attenuation->TryGetNumberField("ReverbDistanceMax", ReverbDistanceMax)) 
			SoundAttenuation->Attenuation.ReverbDistanceMax = Attenuation->GetNumberField("ReverbDistanceMax");
		if (int64 ReverbDistanceMin; Attenuation->TryGetNumberField("ReverbDistanceMin", ReverbDistanceMin)) 
			SoundAttenuation->Attenuation.ReverbDistanceMin = Attenuation->GetNumberField("ReverbDistanceMin");
		if (int64 ReverbWetLevelMax; Attenuation->TryGetNumberField("ReverbWetLevelMax", ReverbWetLevelMax)) 
			SoundAttenuation->Attenuation.ReverbWetLevelMax = Attenuation->GetNumberField("ReverbWetLevelMax");
		if (int64 ReverbWetLevelMin; Attenuation->TryGetNumberField("ReverbWetLevelMin", ReverbWetLevelMin)) 
			SoundAttenuation->Attenuation.ReverbWetLevelMin = Attenuation->GetNumberField("ReverbWetLevelMin");
		if (int64 StereoSpread; Attenuation->TryGetNumberField("StereoSpread", StereoSpread)) 
			SoundAttenuation->Attenuation.StereoSpread = Attenuation->GetNumberField("StereoSpread");

		//Strings
		if (FString AbsorptionMethod; Attenuation->TryGetStringField("AbsorptionMethod", AbsorptionMethod)) 
			SoundAttenuation->Attenuation.AbsorptionMethod = static_cast<EAirAbsorptionMethod>(StaticEnum<EAirAbsorptionMethod>()->GetValueByNameString(AbsorptionMethod));
		if (FString PriorityAttenuationMethod; Attenuation->TryGetStringField("PriorityAttenuationMethod", PriorityAttenuationMethod)) 
			SoundAttenuation->Attenuation.PriorityAttenuationMethod = static_cast<EPriorityAttenuationMethod>(StaticEnum<EPriorityAttenuationMethod>()->GetValueByNameString(PriorityAttenuationMethod));
		if (FString ReverbSendMethod; Attenuation->TryGetStringField("ReverbSendMethod", ReverbSendMethod)) 
			SoundAttenuation->Attenuation.ReverbSendMethod = static_cast<EReverbSendMethod>(StaticEnum<EReverbSendMethod>()->GetValueByNameString(ReverbSendMethod));
		if (FString FalloffMode; Attenuation->TryGetStringField("FalloffMode", FalloffMode))
			SoundAttenuation->Attenuation.FalloffMode = static_cast<ENaturalSoundFalloffMode>(StaticEnum<ENaturalSoundFalloffMode>()->GetValueByNameString(FalloffMode));
		if (FString DistanceAlgorithm; Attenuation->TryGetStringField("DistanceAlgorithm", DistanceAlgorithm)) 
			SoundAttenuation->Attenuation.DistanceAlgorithm = static_cast<EAttenuationDistanceModel>(StaticEnum<EAttenuationDistanceModel>()->GetValueByNameString(DistanceAlgorithm));
		if (FString SpatializationAlgorithm; Attenuation->TryGetStringField("SpatializationAlgorithm", SpatializationAlgorithm)) 
			SoundAttenuation->Attenuation.SpatializationAlgorithm = static_cast<ESoundSpatializationAlgorithm>(StaticEnum<ESoundSpatializationAlgorithm>()->GetValueByNameString(SpatializationAlgorithm));
		if (FString AttenuationShape; Attenuation->TryGetStringField("AttenuationShape", AttenuationShape)) 
			SoundAttenuation->Attenuation.AttenuationShape = static_cast<EAttenuationShape::Type>(StaticEnum<EAttenuationShape::Type>()->GetValueByNameString(AttenuationShape));
		if (FString OcclusionTraceChannel; Attenuation->TryGetStringField("OcclusionTraceChannel", OcclusionTraceChannel)) 
			SoundAttenuation->Attenuation.OcclusionTraceChannel = static_cast<ECollisionChannel>(StaticEnum<ECollisionChannel>()->GetValueByNameString(OcclusionTraceChannel));

		if (const TSharedPtr<FJsonObject>* AttenuationShapeExtents; Attenuation->TryGetObjectField("AttenuationShapeExtents", AttenuationShapeExtents)) 
			SoundAttenuation->Attenuation.AttenuationShapeExtents = FMathUtilities::ObjectToVector(AttenuationShapeExtents->Get());

		if (const TSharedPtr<FJsonObject>* CustomAttenuationCurve; Attenuation->TryGetObjectField("CustomAttenuationCurve", CustomAttenuationCurve)) {
			TArray<TSharedPtr<FJsonValue>> Keys = Attenuation->GetObjectField("CustomAttenuationCurve")->GetObjectField("EditorCurveData")->GetArrayField("Keys");
			FRichCurve Curve;

			for (int32 i = 0; i < Keys.Num(); i++) {
				Curve.Keys.Add(FMathUtilities::ObjectToRichCurveKey(Keys[i]->AsObject()));
			}

			SoundAttenuation->Attenuation.CustomAttenuationCurve.EditorCurveData = Curve;
		}

		// Handle edit changes, and add it to the content browser
		HandleAssetCreation(SoundAttenuation);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
