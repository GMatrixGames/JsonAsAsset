// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/AnimationBaseImporter.h"

#include "UObject/Object.h"
#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/MathUtilities.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimCurveCompressionCodec_CompressedRichCurve.h"

bool UAnimationBaseImporter::ImportData() {
	try {
		// Properties of the object
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		TArray<TSharedPtr<FJsonValue>> FloatCurves;
		TArray<TSharedPtr<FJsonValue>> Notifies;

		UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(FAssetUtilities::GetSelectedAsset());

		// Some FModel versions have different named objects for curves
		const TSharedPtr<FJsonObject>* RawCurveData;

		if (Properties->TryGetObjectField("RawCurveData", RawCurveData)) FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
		else if (JsonObject->TryGetObjectField("CompressedCurveData", RawCurveData)) FloatCurves = JsonObject->GetObjectField("CompressedCurveData")->GetArrayField("FloatCurves");

		for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves) {
			// Display Name (for example: jaw_open_pose)
			FString DisplayName = "";
			if (FloatCurveObject->AsObject()->HasField("Name")) {
				DisplayName = FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName");
			} else {
				DisplayName = FloatCurveObject->AsObject()->GetStringField("CurveName");
			}
			GLog->Log("JsonAsAsset: Added animation curve: " + DisplayName);

			USkeleton* Skeleton = AnimSequenceBase->GetSkeleton();
			FSmartName NewTrackName;

			// Included to add the curve's name to the skeleton's data
			Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, FName(*DisplayName), NewTrackName);

			// Curve Type Flags:
			//	Used to define if a curve
			//  if a curve is metadata
			//  or not.
			int CurveTypeFlags = FloatCurveObject->AsObject()->GetIntegerField("CurveTypeFlags");
			ensureAlways(Skeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, NewTrackName.UID, NewTrackName));

			AnimSequenceBase->RawCurveData.AddCurveData(NewTrackName, CurveTypeFlags);

			// Each key of the curve
			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");

			for (int32 key_index = 0; key_index < Keys.Num(); key_index++) {
				TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();

				FRichCurveKey RichKey = FMathUtilities::ObjectToRichCurveKey(Keys[key_index]->AsObject());

				AnimSequenceBase->RawCurveData.AddFloatCurveKey(NewTrackName, CurveTypeFlags, RichKey.Time, RichKey.Value);
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().ArriveTangent = RichKey.ArriveTangent;
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().LeaveTangent = RichKey.LeaveTangent;
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().InterpMode = RichKey.InterpMode;
			}
		}

		UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);
		const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1;

		if (Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence) {
			TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

			for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers) {
				FAnimSyncMarker AuthoredSyncMarker = FAnimSyncMarker();
				AuthoredSyncMarker.MarkerName = FName(*SyncMarker.Get()->AsObject().Get()->GetStringField("MarkerName"));
				AuthoredSyncMarker.Time = SyncMarker.Get()->AsObject().Get()->GetNumberField("Time");
				CastedAnimSequence->AuthoredSyncMarkers.Add(AuthoredSyncMarker);
			}
		}

		if (CastedAnimSequence) {
			CastedAnimSequence->CurveCompressionCodec = NewObject<UAnimCurveCompressionCodec_CompressedRichCurve>(CastedAnimSequence, TEXT("CurveCompressionCodec"));
			CastedAnimSequence->CurveCompressionCodec->SetFlags(RF_Transactional);
			CastedAnimSequence->MarkRawDataAsModified();
			CastedAnimSequence->RequestSyncAnimRecompression();
		}

		AnimSequenceBase->Modify();
		AnimSequenceBase->PostEditChange();
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
