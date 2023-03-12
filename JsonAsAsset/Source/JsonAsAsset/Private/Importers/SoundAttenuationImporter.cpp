// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundAttenuationImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/MathUtilities.h"

bool USoundAttenuationImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		USoundAttenuation* SoundAttenuation = NewObject<USoundAttenuation>(Package, USoundAttenuation::StaticClass(), *FileName, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(SoundAttenuation);
		if (!SoundAttenuation->MarkPackageDirty()) return false;
		Package->SetDirtyFlag(true);
		SoundAttenuation->PostEditChange();
		SoundAttenuation->AddToRoot();

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
		int64 OmniRadius;
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
		if (Attenuation->TryGetNumberField("OmniRadius", OmniRadius)) {
			SoundAttenuation->Attenuation.OmniRadius = Attenuation->GetNumberField("OmniRadius");
		}
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
		FString StringOcclusionTraceChannel;
		FString StringPriorityAttenuationMethod;
		FString StringReverbSendMethod;
		FString StringSpatializationAlgorithm;
		FString StringAttenuationShape;
		FString StringDistanceAlgorithm;
		FString StringFalloffMode;

		if (Attenuation->TryGetStringField("AbsorptionMethod", AbsorptionMethod)) {
			SoundAttenuation->Attenuation.AbsorptionMethod = AbsorptionMethod.EndsWith("CustomCurve") ? EAirAbsorptionMethod::CustomCurve : EAirAbsorptionMethod::Linear;
		}
		if (Attenuation->TryGetStringField("PriorityAttenuationMethod", StringPriorityAttenuationMethod)) {
			EPriorityAttenuationMethod PriorityAttenuationMethod;

			if (StringPriorityAttenuationMethod.EndsWith("CustomCurve")) PriorityAttenuationMethod = EPriorityAttenuationMethod::CustomCurve;
			else if (StringPriorityAttenuationMethod.EndsWith("Manual")) PriorityAttenuationMethod = EPriorityAttenuationMethod::Manual;
			else PriorityAttenuationMethod = EPriorityAttenuationMethod::Linear;

			SoundAttenuation->Attenuation.PriorityAttenuationMethod = PriorityAttenuationMethod;
		}
		if (Attenuation->TryGetStringField("ReverbSendMethod", StringReverbSendMethod)) {
			EReverbSendMethod ReverbSendMethod;

			if (StringReverbSendMethod.EndsWith("CustomCurve")) ReverbSendMethod = EReverbSendMethod::CustomCurve;
			else if (StringReverbSendMethod.EndsWith("Manual")) ReverbSendMethod = EReverbSendMethod::Manual;
			else ReverbSendMethod = EReverbSendMethod::Linear;

			SoundAttenuation->Attenuation.ReverbSendMethod = ReverbSendMethod;
		}
		if (Attenuation->TryGetStringField("FalloffMode", StringFalloffMode)) {
			ENaturalSoundFalloffMode FalloffMode;

			if (StringFalloffMode.EndsWith("Hold")) FalloffMode = ENaturalSoundFalloffMode::Hold;
			else if (StringFalloffMode.EndsWith("Silent")) FalloffMode = ENaturalSoundFalloffMode::Silent;
			else FalloffMode = ENaturalSoundFalloffMode::Continues;

			SoundAttenuation->Attenuation.FalloffMode = FalloffMode;
		}
		if (Attenuation->TryGetStringField("DistanceAlgorithm", StringDistanceAlgorithm)) {
			EAttenuationDistanceModel DistanceAlgorithm;

			if (StringDistanceAlgorithm.EndsWith("Logarithmic")) DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
			else if (StringDistanceAlgorithm.EndsWith("Inverse")) DistanceAlgorithm = EAttenuationDistanceModel::Inverse;
			else if (StringDistanceAlgorithm.EndsWith("LogReverse")) DistanceAlgorithm = EAttenuationDistanceModel::LogReverse;
			else if (StringDistanceAlgorithm.EndsWith("NaturalSound")) DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
			else if (StringDistanceAlgorithm.EndsWith("Custom")) DistanceAlgorithm = EAttenuationDistanceModel::Custom;
			else DistanceAlgorithm = EAttenuationDistanceModel::Linear;

			SoundAttenuation->Attenuation.DistanceAlgorithm = DistanceAlgorithm;
		}
		if (Attenuation->TryGetStringField("SpatializationAlgorithm", StringSpatializationAlgorithm)) {
			ESoundSpatializationAlgorithm SpatializationAlgorithm;

			if (StringSpatializationAlgorithm.EndsWith("SPATIALIZATION_HRTF")) SpatializationAlgorithm = SPATIALIZATION_HRTF;
			else SpatializationAlgorithm = SPATIALIZATION_Default;

			SoundAttenuation->Attenuation.SpatializationAlgorithm = SpatializationAlgorithm;
		}
		if (Attenuation->TryGetStringField("AttenuationShape", StringAttenuationShape)) {
			EAttenuationShape::Type AttenuationShape;

			if (StringAttenuationShape.EndsWith("Capsule")) AttenuationShape = EAttenuationShape::Capsule;
			else if (StringAttenuationShape.EndsWith("Box")) AttenuationShape = EAttenuationShape::Box;
			else if (StringAttenuationShape.EndsWith("Cone")) AttenuationShape = EAttenuationShape::Cone;
			else AttenuationShape = EAttenuationShape::Sphere;

			SoundAttenuation->Attenuation.AttenuationShape = AttenuationShape;
		}
		if (Attenuation->TryGetStringField("OcclusionTraceChannel", StringOcclusionTraceChannel)) {
			ECollisionChannel OcclusionTraceChannel;

			if (StringOcclusionTraceChannel.EndsWith("ECC_WorldStatic")) OcclusionTraceChannel = ECC_WorldStatic;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_WorldDynamic")) OcclusionTraceChannel = ECC_WorldDynamic;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_Pawn")) OcclusionTraceChannel = ECC_Pawn;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_Visibility")) OcclusionTraceChannel = ECC_Visibility;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_Camera")) OcclusionTraceChannel = ECC_Camera;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_PhysicsBody")) OcclusionTraceChannel = ECC_PhysicsBody;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_Vehicle")) OcclusionTraceChannel = ECC_Vehicle;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_Destructible")) OcclusionTraceChannel = ECC_Destructible;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel1")) OcclusionTraceChannel = ECC_EngineTraceChannel1;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel2")) OcclusionTraceChannel = ECC_EngineTraceChannel2;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel3")) OcclusionTraceChannel = ECC_EngineTraceChannel3;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel4")) OcclusionTraceChannel = ECC_EngineTraceChannel4;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel5")) OcclusionTraceChannel = ECC_EngineTraceChannel5;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel6")) OcclusionTraceChannel = ECC_EngineTraceChannel6;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel1")) OcclusionTraceChannel = ECC_GameTraceChannel1;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel2")) OcclusionTraceChannel = ECC_GameTraceChannel2;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel3")) OcclusionTraceChannel = ECC_GameTraceChannel3;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel4")) OcclusionTraceChannel = ECC_GameTraceChannel4;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel5")) OcclusionTraceChannel = ECC_GameTraceChannel5;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel6")) OcclusionTraceChannel = ECC_GameTraceChannel6;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel7")) OcclusionTraceChannel = ECC_GameTraceChannel7;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel8")) OcclusionTraceChannel = ECC_GameTraceChannel8;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel9")) OcclusionTraceChannel = ECC_GameTraceChannel9;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel10")) OcclusionTraceChannel = ECC_GameTraceChannel10;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel11")) OcclusionTraceChannel = ECC_GameTraceChannel11;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel12")) OcclusionTraceChannel = ECC_GameTraceChannel12;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel13")) OcclusionTraceChannel = ECC_GameTraceChannel13;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel14")) OcclusionTraceChannel = ECC_GameTraceChannel14;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel15")) OcclusionTraceChannel = ECC_GameTraceChannel15;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel16")) OcclusionTraceChannel = ECC_GameTraceChannel16;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel17")) OcclusionTraceChannel = ECC_GameTraceChannel17;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_GameTraceChannel18")) OcclusionTraceChannel = ECC_GameTraceChannel18;
			else if (StringOcclusionTraceChannel.EndsWith("ECC_OverlapAll_Deprecated")) OcclusionTraceChannel = ECC_OverlapAll_Deprecated;
			else OcclusionTraceChannel = ECC_Visibility;

			SoundAttenuation->Attenuation.OcclusionTraceChannel = OcclusionTraceChannel;
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
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
