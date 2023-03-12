// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/AnimationBaseImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Animation/AnimSequence.h"
#include "Utilities/MathUtilities.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

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
		TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers;

		const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1;
		const TSharedPtr<FJsonObject>* RawCurveData;

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> SelectedAssets;
		ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

		UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(SelectedAssets[0].GetAsset());

		// For Unreal Engine 5, a controller is needed
		#if ENGINE_MAJOR_VERSION == 5
			/* In Unreal Engine 5, a new data model has been added to edit animation curves */
			UAnimDataModel* Model = AnimSequenceBase->GetDataModel();
			checkf(Model != nullptr, TEXT("Invalid UAnimDataModel Object"));
			Model->GetAnimationSequence();

			TStrongObjectPtr<UAnimDataController> Controller(NewObject<UAnimDataController>());
			Controller->SetModel(Model);
		#endif

		// Some FModel versions have different named objects for curves
		if (Properties->TryGetObjectField("RawCurveData", RawCurveData)) FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
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
				Controller->AddCurve(FAnimationCurveIdentifier(NewTrackName, ERawCurveTrackTypes::RCT_Float), CurveTypeFlags);
			#endif
			#if ENGINE_MAJOR_VERSION == 4
				AnimSequenceBase->RawCurveData.AddCurveData(NewTrackName, CurveTypeFlags);
			#endif

			// Each key of the curve
			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");

			for (int32 key_index = 0; key_index < Keys.Num(); key_index++) {
				TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();
				ERichCurveInterpMode InterpMode =
					Key->GetStringField("InterpMode") == "RCIM_Linear" ? ERichCurveInterpMode::RCIM_Linear : // Linear
					Key->GetStringField("InterpMode") == "RCIM_Cubic" ? ERichCurveInterpMode::RCIM_Cubic : // Cubic
					Key->GetStringField("InterpMode") == "RCIM_Constant" ? ERichCurveInterpMode::RCIM_Constant : // Constant
					ERichCurveInterpMode::RCIM_None;

				FRichCurveKey RichKey = FRichCurveKey(float(Key->GetNumberField("Time")), float(Key->GetNumberField("Value")), float(Key->GetNumberField("ArriveTangent")), float(Key->GetNumberField("LeaveTangent")), InterpMode);

				// Unreal Engine 5 and Unreal Engine 4
				// have different ways of adding curves
				//
				// Unreal Engine 4: Simply adding curves to RawCurveData
				// Unreal Engine 5: Using a AnimDataController to handle adding curves
				#if ENGINE_MAJOR_VERSION == 5
					const FAnimationCurveIdentifier CurveId(NewTrackName, ERawCurveTrackTypes::RCT_Float);

					RichKey.Time = float(Key->GetNumberField("Time"));
					RichKey.Value = float(Key->GetNumberField("Value"));
					RichKey.ArriveTangent = float(Key->GetNumberField("ArriveTangent"));
					RichKey.LeaveTangent = float(Key->GetNumberField("LeaveTangent"));
					RichKey.InterpMode = InterpMode;

					Controller->SetCurveKey(CurveId, RichKey);
				#endif
				#if ENGINE_MAJOR_VERSION == 4
					AnimSequenceBase->RawCurveData.AddFloatCurveKey(NewTrackName, CurveTypeFlags, float(Key->GetNumberField("Time")), float(Key->GetNumberField("Value")));
					AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().ArriveTangent = float(Key->GetNumberField("ArriveTangent"));
					AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().LeaveTangent = float(Key->GetNumberField("LeaveTangent"));
					AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().InterpMode = InterpMode;
				#endif
			}
		}

		UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);

		if (Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence) {
			AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

			for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers) {
				FAnimSyncMarker AuthoredSyncMarker = FAnimSyncMarker();
				AuthoredSyncMarker.MarkerName = FName(*SyncMarker.Get()->AsObject().Get()->GetStringField("MarkerName"));
				AuthoredSyncMarker.Time = SyncMarker.Get()->AsObject().Get()->GetNumberField("Time");
				CastedAnimSequence->AuthoredSyncMarkers.Add(AuthoredSyncMarker);
			}
		}

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
