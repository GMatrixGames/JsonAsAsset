// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/CurveTableImporter.h"

#include "Dom/JsonObject.h"
#include "Factories/CurveTableFactory.h"
#include "Utilities/AssetUtilities.h"

bool UCurveTableImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> RowData = JsonObject->GetObjectField("Rows");
		UCurveTable* CurveTable = NewObject<UCurveTable>(Package, UCurveTable::StaticClass(), *FileName, RF_Public | RF_Standalone);

		ECurveTableMode CurveTableMode = ECurveTableMode::Empty;
		if (FString CurveMode; JsonObject->TryGetStringField("CurveTableMode", CurveMode)) {
			CurveTableMode = static_cast<ECurveTableMode>(StaticEnum<ECurveTableMode>()->GetValueByNameString(CurveMode));
		}

		// Loop throughout row data, and deserialize
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : RowData->Values) {
			const TSharedPtr<FJsonObject> CurveData = Pair.Value->AsObject();

			if (CurveTableMode == ECurveTableMode::SimpleCurves) {
				FSimpleCurve NewCurve = CurveTable->AddSimpleCurve(*Pair.Key);
				
				if (FString InterpMode; CurveData->TryGetStringField("InterpMode", InterpMode)) {
					NewCurve.InterpMode = static_cast<ERichCurveInterpMode>(StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(InterpMode));
				}
				if (const TArray<TSharedPtr<FJsonValue>>* Keys; CurveData->TryGetArrayField("Keys", Keys)) {
					for (const TSharedPtr<FJsonValue> Key : *Keys) {
						const TSharedPtr<FJsonObject> KeyObj = Key->AsObject();
						NewCurve.Keys.Add(FSimpleCurveKey(KeyObj->GetNumberField("Time"), KeyObj->GetNumberField("Value")));
					}
				}
				if (float DefaultValue; CurveData->TryGetNumberField("DefaultValue", DefaultValue)) {
					NewCurve.DefaultValue = DefaultValue;
				}
				if (FString PreInfinityExtrap; CurveData->TryGetStringField("PreInfinityExtrap", PreInfinityExtrap)) {
					NewCurve.PreInfinityExtrap = static_cast<ERichCurveExtrapolation>(StaticEnum<ERichCurveExtrapolation>()->GetValueByNameString(PreInfinityExtrap));
				}
				if (FString PostInfinityExtrap; CurveData->TryGetStringField("PostInfinityExtrap", PostInfinityExtrap)) {
					NewCurve.PostInfinityExtrap = static_cast<ERichCurveExtrapolation>(StaticEnum<ERichCurveExtrapolation>()->GetValueByNameString(PostInfinityExtrap));
				}
			}
			// } else {
			// 	FRichCurve* NewCurve = new FRichCurve();
			// 	// Now iterate over cells (skipping first cell, that was row name)
			// 	for (int32 ColumnIdx = 0; ColumnIdx < XValues.Num(); ColumnIdx++) {
			// 		FKeyHandle KeyHandle = NewCurve->AddKey(XValues[ColumnIdx], YValues[ColumnIdx]);
			// 		NewCurve->SetKeyInterpMode(KeyHandle, InterpMode);
			// 	}
			//
			// 	RowMap.Add(RowName, NewCurve);
			// }
		
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(CurveTable)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
