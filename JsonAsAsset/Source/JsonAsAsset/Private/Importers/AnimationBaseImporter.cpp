// Copyright JAA Contributors 2023-2024

#include "Importers/AnimationBaseImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/MathUtilities.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"

#if ENGINE_MAJOR_VERSION == 5
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimDataController.h"
#endif

bool UAnimationBaseImporter::ImportData()
{
	try
	{
		// Properties of the object
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		TArray<TSharedPtr<FJsonValue>> FloatCurves;
		TArray<TSharedPtr<FJsonValue>> Notifies;

		UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(FAssetUtilities::GetSelectedAsset());

		/* In Unreal Engine 5, a new data model has been added to edit animation curves */
		// Unreal Engine 5.2 changed handling getting a data model
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		IAnimationDataController& Controller = AnimSequenceBase->GetController();
#endif

		// Some FModel versions have different named objects for curves
		if (const TSharedPtr<FJsonObject>* RawCurveData; Properties->TryGetObjectField("RawCurveData", RawCurveData)) FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
		else if (JsonObject->TryGetObjectField("CompressedCurveData", RawCurveData)) FloatCurves = JsonObject->GetObjectField("CompressedCurveData")->GetArrayField("FloatCurves");

		for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves)
		{
			// Display Name (for example: jaw_open_pose)
			FString DisplayName = FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName");

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
			// For Unreal Engine 5.3 and above, the smart name's display name is required
#if ENGINE_MINOR_VERSION == 3 && ENGINE_PATCH_VERSION < 2
			Controller->AddCurve(FAnimationCurveIdentifier(NewTrackName.DisplayName, ERawCurveTrackTypes::RCT_Float), CurveTypeFlags);
#elif (ENGINE_MINOR_VERSION == 3 && ENGINE_PATCH_VERSION == 2) || ENGINE_MINOR_VERSION > 3
			Controller.AddCurve(FAnimationCurveIdentifier(NewTrackName.DisplayName, ERawCurveTrackTypes::RCT_Float), CurveTypeFlags);
#endif
			// For Unreal Engine 5.2 and below, just the smart name is required
#if ENGINE_MINOR_VERSION < 3
			AnimSequenceBase->Modify(true);

			IAnimationDataController& LocalOneController = AnimSequenceBase->GetController();
			LocalOneController.AddCurve(FAnimationCurveIdentifier(NewTrackName, ERawCurveTrackTypes::RCT_Float), CurveTypeFlags);

			TArray<FLinearColor> RandomizedColorArray = {
				FLinearColor(.904, .323, .539),
				FLinearColor(.552, .737, .328),
				FLinearColor(.947, .418, .219),
				FLinearColor(.156, .624, .921),
				FLinearColor(.921, .314, .337),
				FLinearColor(.361, .651, .332),
				FLinearColor(.982, .565, .254),
				FLinearColor(.246, .223, .514),
				FLinearColor(.208, .386, .687),
				FLinearColor(.223, .590, .337),
				FLinearColor(.230, .291, .591)
			}; { // Random Color
				auto index = rand() % RandomizedColorArray.Num();

				if (RandomizedColorArray.IsValidIndex(index)) // Safe
					LocalOneController.SetCurveColor(FAnimationCurveIdentifier(NewTrackName, ERawCurveTrackTypes::RCT_Float), RandomizedColorArray[index]);
			}

			AnimSequenceBase->PostEditChange();
#endif
#endif
#if ENGINE_MAJOR_VERSION == 4
			AnimSequenceBase->RawCurveData.AddCurveData(NewTrackName, CurveTypeFlags);
#endif
		}

		for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves)
		{
			FString DisplayName = FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName");

			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");
			TArray<FRichCurveKey> _Keys;

			for (int32 key_index = 0; key_index < Keys.Num(); key_index++)
			{
				TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();

				FRichCurveKey RichKey = FMathUtilities::ObjectToRichCurveKey(Keys[key_index]->AsObject());

				// Unreal Engine 5 and Unreal Engine 4
				// have different ways of adding curves
				//
				// Unreal Engine 4: Simply adding curves to RawCurveData
				// Unreal Engine 5: Using a AnimDataController to handle adding curves
#if ENGINE_MAJOR_VERSION == 5
				_Keys.Add(RichKey);
#endif
#if ENGINE_MAJOR_VERSION == 4
				AnimSequenceBase->RawCurveData.AddFloatCurveKey(NewTrackName, CurveTypeFlags, RichKey.Time, RichKey.Value);
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().ArriveTangent = RichKey.ArriveTangent;
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().LeaveTangent = RichKey.LeaveTangent;
				AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().InterpMode = RichKey.InterpMode;
#endif
			}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
			FSmartName NewTrackName;
			{
				// Create SmartName
				USkeleton* Skeleton = AnimSequenceBase->GetSkeleton();
				Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, FName(*DisplayName), NewTrackName);
			}

			AnimSequenceBase->Modify(true);

			FAnimationCurveIdentifier CurveId(NewTrackName, ERawCurveTrackTypes::RCT_Float);
			IAnimationDataController& LocalController = AnimSequenceBase->GetController();
			LocalController.SetCurveKeys(CurveId, _Keys);

			AnimSequenceBase->PostEditChange();
			GLog->Log("JsonAsAsset: Added animation curve: " + DisplayName);
#endif
		}

		UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);

		if (const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1; Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence)
		{
			TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

			for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers)
			{
				FAnimSyncMarker AuthoredSyncMarker = FAnimSyncMarker();
				AuthoredSyncMarker.MarkerName = FName(*SyncMarker.Get()->AsObject().Get()->GetStringField("MarkerName"));
				AuthoredSyncMarker.Time = SyncMarker.Get()->AsObject().Get()->GetNumberField("Time");
				CastedAnimSequence->AuthoredSyncMarkers.Add(AuthoredSyncMarker);
			}
		}

		// TODO: Incompatible with UE5
		if (CastedAnimSequence)
		{
			CastedAnimSequence->RequestSyncAnimRecompression();
		}

		UAnimMontage* CastedAnimMontage = Cast<UAnimMontage>(AnimSequenceBase);

#if ENGINE_MAJOR_VERSION == 4
		AnimSequenceBase->MarkRawDataAsModified();
#endif
		AnimSequenceBase->Modify();
		AnimSequenceBase->PostEditChange();

		SavePackage();
	}
	catch (const char* Exception)
	{
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
