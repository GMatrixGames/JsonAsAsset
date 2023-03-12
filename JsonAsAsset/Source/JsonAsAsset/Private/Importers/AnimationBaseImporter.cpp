// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/AnimationBaseImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Animation/AnimSequence.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Animation/AnimMontage.h"

#if ENGINE_MAJOR_VERSION == 5
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimDataController.h"
#endif

bool UAnimationBaseImporter::ImportData() {
	try {
		// Properties of the object
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		TArray<TSharedPtr<FJsonValue>> FloatCurves;
		TArray<TSharedPtr<FJsonValue>> Notifies;

		UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(FAssetUtilities::GetSelectedAsset());

		// For Unreal Engine 5, a controller is needed
#if ENGINE_MAJOR_VERSION == 5
		/* In Unreal Engine 5, a new data model has been added to edit animation curves */
		TScriptInterface<IAnimationDataModel> Model = AnimSequenceBase->GetDataModelInterface();
		checkf(Model != nullptr, TEXT("Invalid UAnimDataModel Object"));
		Model->GetAnimationSequence();

		TStrongObjectPtr Controller(NewObject<UAnimDataController>());
		Controller->SetModel(Model);
#endif

		// Some FModel versions have different named objects for curves
		if (const TSharedPtr<FJsonObject>* RawCurveData; Properties->TryGetObjectField("RawCurveData", RawCurveData)) FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
		else if (JsonObject->TryGetObjectField("CompressedCurveData", RawCurveData)) FloatCurves = JsonObject->GetObjectField("CompressedCurveData")->GetArrayField("FloatCurves");

		for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves) {
			// Display Name (for example: jaw_open_pose)
			FString DisplayName = FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName");
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

			// Unreal Engine 5 uses the controller, while Unreal Engine 4 directly uses RawCurveData
#if ENGINE_MAJOR_VERSION == 5
			Controller->AddCurve(FAnimationCurveIdentifier(NewTrackName.DisplayName, ERawCurveTrackTypes::RCT_Float), CurveTypeFlags);
#endif
#if ENGINE_MAJOR_VERSION == 4
			AnimSequenceBase->RawCurveData.AddCurveData(NewTrackName, CurveTypeFlags);
#endif

			// Each key of the curve
			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");

			for (int32 key_index = 0; key_index < Keys.Num(); key_index++) {
				TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();

				FRichCurveKey RichKey = FAssetUtilities::ObjectToRichCurveKey(Keys[key_index]->AsObject());

				// Unreal Engine 5 and Unreal Engine 4
				// have different ways of adding curves
				//
				// Unreal Engine 4: Simply adding curves to RawCurveData
				// Unreal Engine 5: Using a AnimDataController to handle adding curves
#if ENGINE_MAJOR_VERSION == 5
				const FAnimationCurveIdentifier CurveId(NewTrackName, ERawCurveTrackTypes::RCT_Float);
				Controller->SetCurveKey(CurveId, RichKey);
#endif
#if ENGINE_MAJOR_VERSION == 4
				AnimSequenceBase->RawCurveData.AddFloatCurveKey(NewTrackName, CurveTypeFlags, RichKey.Time, RichKey.Value);
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().ArriveTangent = RichKey.ArriveTangent;
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().LeaveTangent = RichKey.LeaveTangent;
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().InterpMode = RichKey.InterpMode;
#endif
			}
		}

		UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);

		if (const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1; Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence) {
			TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

			for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers) {
				FAnimSyncMarker AuthoredSyncMarker = FAnimSyncMarker();
				AuthoredSyncMarker.MarkerName = FName(*SyncMarker.Get()->AsObject().Get()->GetStringField("MarkerName"));
				AuthoredSyncMarker.Time = SyncMarker.Get()->AsObject().Get()->GetNumberField("Time");
				CastedAnimSequence->AuthoredSyncMarkers.Add(AuthoredSyncMarker);
			}
		}

		// TODO: Incompatible with UE5
		if (CastedAnimSequence) {
			CastedAnimSequence->MarkRawDataAsModified();
			CastedAnimSequence->RequestSyncAnimRecompression();
		}

		UAnimMontage* CastedAnimMontage = Cast<UAnimMontage>(AnimSequenceBase);

		AnimSequenceBase->MarkRawDataAsModified();
		AnimSequenceBase->Modify();
		AnimSequenceBase->PostEditChange();
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
