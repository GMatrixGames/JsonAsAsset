// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundAttenuationImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/MathUtilities.h"

bool USoundAttenuationImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		USoundAttenuation* SoundAttenuation = NewObject<USoundAttenuation>(Package, USoundAttenuation::StaticClass(), *FileName, RF_Public | RF_Standalone);
		TSharedPtr<FJsonObject> Attenuation = Properties->GetObjectField("Attenuation");

		bool bApplyNormalizationToStereoSounds;
		bool bAttenuate;
		bool bAttenuateWithLPF;
		bool bEnableFocusInterpolation;
		bool bEnableListenerFocus;
		bool bEnableLogFrequencyScaling;
		bool bEnableOcclusion;
		bool bEnablePriorityAttenuation;
		bool bEnableReverbSend;
		bool bEnableSubmixSends;
		bool bSpatialize;
		bool bUseComplexCollisionForOcclusion;

		if (Attenuation->TryGetBoolField("bApplyNormalizationToStereoSounds", bApplyNormalizationToStereoSounds)) {
			SoundAttenuation->Attenuation.bApplyNormalizationToStereoSounds = Attenuation->GetBoolField("bApplyNormalizationToStereoSounds");
		}
		if (Attenuation->TryGetBoolField("bAttenuate", bAttenuate)) {
			SoundAttenuation->Attenuation.bAttenuate = Attenuation->GetBoolField("bAttenuate");
		}
		if (Attenuation->TryGetBoolField("bAttenuateWithLPF", bAttenuateWithLPF)) {
			SoundAttenuation->Attenuation.bAttenuateWithLPF = Attenuation->GetBoolField("bAttenuateWithLPF");
		}
		if (Attenuation->TryGetBoolField("bEnableFocusInterpolation", bEnableFocusInterpolation)) {
			SoundAttenuation->Attenuation.bEnableFocusInterpolation = Attenuation->GetBoolField("bEnableFocusInterpolation");
		}
		if (Attenuation->TryGetBoolField("bEnableListenerFocus", bEnableListenerFocus)) {
			SoundAttenuation->Attenuation.bEnableListenerFocus = Attenuation->GetBoolField("bEnableListenerFocus");
		}
		if (Attenuation->TryGetBoolField("bEnableLogFrequencyScaling", bEnableLogFrequencyScaling)) {
			SoundAttenuation->Attenuation.bEnableLogFrequencyScaling = Attenuation->GetBoolField("bEnableLogFrequencyScaling");
		}
		if (Attenuation->TryGetBoolField("bEnableOcclusion", bEnableOcclusion)) {
			SoundAttenuation->Attenuation.bEnableOcclusion = Attenuation->GetBoolField("bEnableOcclusion");
		}
		if (Attenuation->TryGetBoolField("bEnablePriorityAttenuation", bEnablePriorityAttenuation)) {
			SoundAttenuation->Attenuation.bEnablePriorityAttenuation = Attenuation->GetBoolField("bEnablePriorityAttenuation");
		}
		if (Attenuation->TryGetBoolField("bEnableReverbSend", bEnableReverbSend)) {
			SoundAttenuation->Attenuation.bEnableReverbSend = Attenuation->GetBoolField("bEnableReverbSend");
		}
		if (Attenuation->TryGetBoolField("bEnableSubmixSends", bEnableSubmixSends)) {
			SoundAttenuation->Attenuation.bEnableSubmixSends = Attenuation->GetBoolField("bEnableSubmixSends");
		}
		if (Attenuation->TryGetBoolField("bSpatialize", bSpatialize)) {
			SoundAttenuation->Attenuation.bSpatialize = Attenuation->GetBoolField("bSpatialize");
		}
		if (Attenuation->TryGetBoolField("bUseComplexCollisionForOcclusion", bUseComplexCollisionForOcclusion)) {
			SoundAttenuation->Attenuation.bUseComplexCollisionForOcclusion = Attenuation->GetBoolField("bUseComplexCollisionForOcclusion");
		}

		int64 BinauralRadius;
		int64 FocusAttackInterpSpeed;
		int64 FocusAzimuth;
		int64 FocusDistanceScale;
		int64 FocusPriorityScale;
		int64 FocusReleaseInterpSpeed;
		int64 FocusVolumeAttenuation;

		int64 HPFFrequencyAtMax;
		int64 HPFFrequencyAtMin;
		int64 LPFRadiusMax;
		int64 LPFRadiusMin;

		int64 ManualPriorityAttenuation;
		int64 ManualReverbSendLevel;
		int64 NonFocusAzimuth;
		int64 NonFocusDistanceScale;
		int64 NonFocusPriorityScale;
		int64 NonFocusVolumeAttenuation;
		int64 OcclusionInterpolationTime;
		int64 OcclusionLowPassFilterFrequency;
		int64 OcclusionVolumeAttenuation;
		int64 PriorityAttenuationDistanceMax;
		int64 PriorityAttenuationDistanceMin;
		int64 PriorityAttenuationMax;
		int64 PriorityAttenuationMin;
		int64 ReverbDistanceMax;
		int64 ReverbDistanceMin;
		int64 ReverbWetLevelMax;
		int64 ReverbWetLevelMin;
		int64 StereoSpread;
		int64 ConeOffset;
		int64 dBAttenuationAtMax;
		int64 FalloffDistance;

		if (Attenuation->TryGetNumberField("ConeOffset", ConeOffset)) {
			SoundAttenuation->Attenuation.ConeOffset = Attenuation->GetNumberField("ConeOffset");
		}
		if (Attenuation->TryGetNumberField("dBAttenuationAtMax", dBAttenuationAtMax)) {
			SoundAttenuation->Attenuation.dBAttenuationAtMax = Attenuation->GetNumberField("dBAttenuationAtMax");
		}
		if (Attenuation->TryGetNumberField("FalloffDistance", FalloffDistance)) {
			SoundAttenuation->Attenuation.FalloffDistance = Attenuation->GetNumberField("FalloffDistance");
		}
		if (Attenuation->TryGetNumberField("BinauralRadius", BinauralRadius)) {
			SoundAttenuation->Attenuation.BinauralRadius = Attenuation->GetNumberField("BinauralRadius");
		}
		if (Attenuation->TryGetNumberField("FocusAttackInterpSpeed", FocusAttackInterpSpeed)) {
			SoundAttenuation->Attenuation.FocusAttackInterpSpeed = Attenuation->GetNumberField("FocusAttackInterpSpeed");
		}
		if (Attenuation->TryGetNumberField("FocusAzimuth", FocusAzimuth)) {
			SoundAttenuation->Attenuation.FocusAzimuth = Attenuation->GetNumberField("FocusAzimuth");
		}
		if (Attenuation->TryGetNumberField("FocusDistanceScale", FocusDistanceScale)) {
			SoundAttenuation->Attenuation.FocusDistanceScale = Attenuation->GetNumberField("FocusDistanceScale");
		}
		if (Attenuation->TryGetNumberField("FocusPriorityScale", FocusPriorityScale)) {
			SoundAttenuation->Attenuation.FocusPriorityScale = Attenuation->GetNumberField("FocusPriorityScale");
		}
		if (Attenuation->TryGetNumberField("FocusReleaseInterpSpeed", FocusReleaseInterpSpeed)) {
			SoundAttenuation->Attenuation.FocusReleaseInterpSpeed = Attenuation->GetNumberField("FocusReleaseInterpSpeed");
		}
		if (Attenuation->TryGetNumberField("FocusVolumeAttenuation", FocusVolumeAttenuation)) {
			SoundAttenuation->Attenuation.FocusVolumeAttenuation = Attenuation->GetNumberField("FocusVolumeAttenuation");
		}
		if (Attenuation->TryGetNumberField("HPFFrequencyAtMax", HPFFrequencyAtMax)) {
			SoundAttenuation->Attenuation.HPFFrequencyAtMax = Attenuation->GetNumberField("HPFFrequencyAtMax");
		}
		if (Attenuation->TryGetNumberField("HPFFrequencyAtMin", HPFFrequencyAtMin)) {
			SoundAttenuation->Attenuation.HPFFrequencyAtMin = Attenuation->GetNumberField("HPFFrequencyAtMin");
		}
		if (Attenuation->TryGetNumberField("ManualPriorityAttenuation", ManualPriorityAttenuation)) {
			SoundAttenuation->Attenuation.ManualPriorityAttenuation = Attenuation->GetNumberField("ManualPriorityAttenuation");
		}
		if (Attenuation->TryGetNumberField("LPFRadiusMin", LPFRadiusMin)) {
			SoundAttenuation->Attenuation.LPFRadiusMin = Attenuation->GetNumberField("LPFRadiusMin");
		}
		if (Attenuation->TryGetNumberField("LPFRadiusMax", LPFRadiusMax)) {
			SoundAttenuation->Attenuation.LPFRadiusMax = Attenuation->GetNumberField("LPFRadiusMax");
		}
		if (Attenuation->TryGetNumberField("ManualReverbSendLevel", ManualReverbSendLevel)) {
			SoundAttenuation->Attenuation.ManualReverbSendLevel = Attenuation->GetNumberField("ManualReverbSendLevel");
		}
		if (Attenuation->TryGetNumberField("NonFocusAzimuth", NonFocusAzimuth)) {
			SoundAttenuation->Attenuation.NonFocusAzimuth = Attenuation->GetNumberField("NonFocusAzimuth");
		}
		if (Attenuation->TryGetNumberField("NonFocusDistanceScale", NonFocusDistanceScale)) {
			SoundAttenuation->Attenuation.NonFocusDistanceScale = Attenuation->GetNumberField("NonFocusDistanceScale");
		}
		if (Attenuation->TryGetNumberField("NonFocusPriorityScale", NonFocusPriorityScale)) {
			SoundAttenuation->Attenuation.NonFocusPriorityScale = Attenuation->GetNumberField("NonFocusPriorityScale");
		}
		if (Attenuation->TryGetNumberField("NonFocusVolumeAttenuation", NonFocusVolumeAttenuation)) {
			SoundAttenuation->Attenuation.NonFocusVolumeAttenuation = Attenuation->GetNumberField("NonFocusVolumeAttenuation");
		}
		if (Attenuation->TryGetNumberField("OcclusionInterpolationTime", OcclusionInterpolationTime)) {
			SoundAttenuation->Attenuation.OcclusionInterpolationTime = Attenuation->GetNumberField("OcclusionInterpolationTime");
		}
		if (Attenuation->TryGetNumberField("OcclusionLowPassFilterFrequency", OcclusionLowPassFilterFrequency)) {
			SoundAttenuation->Attenuation.OcclusionLowPassFilterFrequency = Attenuation->GetNumberField("OcclusionLowPassFilterFrequency");
		}
		if (Attenuation->TryGetNumberField("OcclusionVolumeAttenuation", OcclusionVolumeAttenuation)) {
			SoundAttenuation->Attenuation.OcclusionVolumeAttenuation = Attenuation->GetNumberField("OcclusionVolumeAttenuation");
		}
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3) || ENGINE_MAJOR_VERSION == 4
		int64 OmniRadius;

		if (Attenuation->TryGetNumberField("OmniRadius", OmniRadius)) {
			SoundAttenuation->Attenuation.OmniRadius = Attenuation->GetNumberField("OmniRadius");
		}
#endif
		if (Attenuation->TryGetNumberField("PriorityAttenuationDistanceMax", PriorityAttenuationDistanceMax)) {
			SoundAttenuation->Attenuation.PriorityAttenuationDistanceMax = Attenuation->GetNumberField("PriorityAttenuationDistanceMax");
		}
		if (Attenuation->TryGetNumberField("PriorityAttenuationDistanceMin", PriorityAttenuationDistanceMin)) {
			SoundAttenuation->Attenuation.PriorityAttenuationDistanceMin = Attenuation->GetNumberField("PriorityAttenuationDistanceMin");
		}
		if (Attenuation->TryGetNumberField("PriorityAttenuationMax", PriorityAttenuationMax)) {
			SoundAttenuation->Attenuation.PriorityAttenuationMax = Attenuation->GetNumberField("PriorityAttenuationMax");
		}
		if (Attenuation->TryGetNumberField("PriorityAttenuationMin", PriorityAttenuationMin)) {
			SoundAttenuation->Attenuation.PriorityAttenuationMin = Attenuation->GetNumberField("PriorityAttenuationMin");
		}
		if (Attenuation->TryGetNumberField("ReverbDistanceMax", ReverbDistanceMax)) {
			SoundAttenuation->Attenuation.ReverbDistanceMax = Attenuation->GetNumberField("ReverbDistanceMax");
		}
		if (Attenuation->TryGetNumberField("ReverbDistanceMin", ReverbDistanceMin)) {
			SoundAttenuation->Attenuation.ReverbDistanceMin = Attenuation->GetNumberField("ReverbDistanceMin");
		}
		if (Attenuation->TryGetNumberField("ReverbWetLevelMax", ReverbWetLevelMax)) {
			SoundAttenuation->Attenuation.ReverbWetLevelMax = Attenuation->GetNumberField("ReverbWetLevelMax");
		}
		if (Attenuation->TryGetNumberField("ReverbWetLevelMin", ReverbWetLevelMin)) {
			SoundAttenuation->Attenuation.ReverbWetLevelMin = Attenuation->GetNumberField("ReverbWetLevelMin");
		}
		if (Attenuation->TryGetNumberField("StereoSpread", StereoSpread)) {
			SoundAttenuation->Attenuation.StereoSpread = Attenuation->GetNumberField("StereoSpread");
		}

		FString AbsorptionMethod;
		FString OcclusionTraceChannel;
		FString PriorityAttenuationMethod;
		FString ReverbSendMethod;
		FString SpatializationAlgorithm;
		FString AttenuationShape;
		FString DistanceAlgorithm;
		FString FalloffMode;

		if (Attenuation->TryGetStringField("AbsorptionMethod", AbsorptionMethod)) {
			SoundAttenuation->Attenuation.AbsorptionMethod = static_cast<EAirAbsorptionMethod>(StaticEnum<EAirAbsorptionMethod>()->GetValueByNameString(AbsorptionMethod));
		}
		if (Attenuation->TryGetStringField("PriorityAttenuationMethod", PriorityAttenuationMethod)) {
			SoundAttenuation->Attenuation.PriorityAttenuationMethod = static_cast<EPriorityAttenuationMethod>(StaticEnum<EPriorityAttenuationMethod>()->GetValueByNameString(PriorityAttenuationMethod));
		}
		if (Attenuation->TryGetStringField("ReverbSendMethod", ReverbSendMethod)) {
			SoundAttenuation->Attenuation.ReverbSendMethod = static_cast<EReverbSendMethod>(StaticEnum<EReverbSendMethod>()->GetValueByNameString(ReverbSendMethod));
		}
		if (Attenuation->TryGetStringField("FalloffMode", FalloffMode)) {
			SoundAttenuation->Attenuation.FalloffMode = static_cast<ENaturalSoundFalloffMode>(StaticEnum<ENaturalSoundFalloffMode>()->GetValueByNameString(FalloffMode));
		}
		if (Attenuation->TryGetStringField("DistanceAlgorithm", DistanceAlgorithm)) {
			SoundAttenuation->Attenuation.DistanceAlgorithm = static_cast<EAttenuationDistanceModel>(StaticEnum<EAttenuationDistanceModel>()->GetValueByNameString(FalloffMode));
		}
		if (Attenuation->TryGetStringField("SpatializationAlgorithm", SpatializationAlgorithm)) {
			SoundAttenuation->Attenuation.SpatializationAlgorithm = static_cast<ESoundSpatializationAlgorithm>(StaticEnum<ESoundSpatializationAlgorithm>()->GetValueByNameString(SpatializationAlgorithm));
		}
		if (Attenuation->TryGetStringField("AttenuationShape", AttenuationShape)) {
			SoundAttenuation->Attenuation.AttenuationShape = static_cast<EAttenuationShape::Type>(StaticEnum<EAttenuationShape::Type>()->GetValueByNameString(AttenuationShape));
		}
		if (Attenuation->TryGetStringField("OcclusionTraceChannel", OcclusionTraceChannel)) {
			SoundAttenuation->Attenuation.OcclusionTraceChannel = static_cast<ECollisionChannel>(StaticEnum<ECollisionChannel>()->GetValueByNameString(OcclusionTraceChannel));
		}

		if (const TSharedPtr<FJsonObject>* AttenuationShapeExtents; Attenuation->TryGetObjectField("AttenuationShapeExtents", AttenuationShapeExtents)) {
			SoundAttenuation->Attenuation.AttenuationShapeExtents = FMathUtilities::ObjectToVector(AttenuationShapeExtents->Get());
		}

		if (const TSharedPtr<FJsonObject>* CustomAttenuationCurve; Attenuation->TryGetObjectField("CustomAttenuationCurve", CustomAttenuationCurve)) {
			TArray<TSharedPtr<FJsonValue>> Keys = Attenuation->GetObjectField("CustomAttenuationCurve")->GetObjectField("EditorCurveData")->GetArrayField("Keys");
			FRichCurve Curve;

			for (int32 i = 0; i < Keys.Num(); i++) {
				Curve.Keys.Add(FAssetUtilities::ObjectToRichCurveKey(Keys[i]->AsObject()));
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
