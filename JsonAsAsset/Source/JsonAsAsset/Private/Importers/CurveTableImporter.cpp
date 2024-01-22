// Copyright JAA Contributors 2023-2024

#include "Importers/CurveTableImporter.h"

#include "Dom/JsonObject.h"
#include "Factories/CurveTableFactory.h"
#include "Utilities/AssetUtilities.h"

bool UCurveTableImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> RowData = JsonObject->GetObjectField("Rows");
		UCurveTable* CurveTable = NewObject<UCurveTable>(Package, UCurveTable::StaticClass(), *FileName, RF_Public | RF_Standalone);
		UCurveTableDerived* DerivedCurveTable = Cast<UCurveTableDerived>(CurveTable);

		// Used to determine curve type
		ECurveTableMode CurveTableMode = ECurveTableMode::RichCurves; {
			if (FString CurveMode; JsonObject->TryGetStringField("CurveTableMode", CurveMode))
				CurveTableMode = static_cast<ECurveTableMode>(StaticEnum<ECurveTableMode>()->GetValueByNameString(CurveMode));

			DerivedCurveTable->ChangeTableMode(CurveTableMode);
		}

		// Loop throughout row data, and deserialize
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : RowData->Values) {
			const TSharedPtr<FJsonObject> CurveData = Pair.Value->AsObject();

			// Curve structure (either simple or rich)
			FRealCurve RealCurve;

			if (CurveTableMode == ECurveTableMode::RichCurves) {
				FRichCurve& NewRichCurve = CurveTable->AddRichCurve(FName(*Pair.Key)); {
					RealCurve = NewRichCurve;
				}

				if (const TArray<TSharedPtr<FJsonValue>>* KeysPtr; CurveData->TryGetArrayField("Keys", KeysPtr))
					for (const TSharedPtr<FJsonValue> KeyPtr : *KeysPtr) {
						TSharedPtr<FJsonObject> Key = KeyPtr->AsObject(); {
							NewRichCurve.AddKey(Key->GetNumberField("Time"), Key->GetNumberField("Value"));
							FRichCurveKey RichKey = NewRichCurve.Keys.Last();

							RichKey.InterpMode =
								static_cast<ERichCurveInterpMode>(
									StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(Key->GetStringField("InterpMode"))
								);
							RichKey.TangentMode =
								static_cast<ERichCurveTangentMode>(
									StaticEnum<ERichCurveTangentMode>()->GetValueByNameString(Key->GetStringField("TangentMode"))
								);
							RichKey.TangentWeightMode =
								static_cast<ERichCurveTangentWeightMode>(
									StaticEnum<ERichCurveTangentWeightMode>()->GetValueByNameString(Key->GetStringField("TangentWeightMode"))
								);

							RichKey.ArriveTangent = Key->GetNumberField("ArriveTangent");
							RichKey.ArriveTangentWeight = Key->GetNumberField("ArriveTangentWeight");
							RichKey.LeaveTangent = Key->GetNumberField("LeaveTangent");
							RichKey.LeaveTangentWeight = Key->GetNumberField("LeaveTangentWeight");
						}
					}
			} else {
				FSimpleCurve& NewSimpleCurve = CurveTable->AddSimpleCurve(FName(*Pair.Key)); {
					RealCurve = NewSimpleCurve;
				}

				// Method of Interpolation
				NewSimpleCurve.InterpMode =
					static_cast<ERichCurveInterpMode>(
						StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(CurveData->GetStringField("InterpMode"))
					);

				if (const TArray<TSharedPtr<FJsonValue>>* KeysPtr; CurveData->TryGetArrayField("Keys", KeysPtr))
					for (const TSharedPtr<FJsonValue> KeyPtr : *KeysPtr) {
						TSharedPtr<FJsonObject> Key = KeyPtr->AsObject(); {
							NewSimpleCurve.AddKey(Key->GetNumberField("Time"), Key->GetNumberField("Value"));
						}
					}
			}

			// Inherited data from FRealCurve
			RealCurve.SetDefaultValue(CurveData->GetNumberField("DefaultValue"));
			RealCurve.PreInfinityExtrap = 
				static_cast<ERichCurveExtrapolation>(
					StaticEnum<ERichCurveExtrapolation>()->GetValueByNameString(CurveData->GetStringField("PreInfinityExtrap"))
				);
			RealCurve.PostInfinityExtrap =
				static_cast<ERichCurveExtrapolation>(
					StaticEnum<ERichCurveExtrapolation>()->GetValueByNameString(CurveData->GetStringField("PostInfinityExtrap"))
				);

			// Update Curve Table
			CurveTable->OnCurveTableChanged().Broadcast();
			CurveTable->Modify(true);
		}

		// Handle edit changes, and add it to the content browser
		SavePackage();
		if (!HandleAssetCreation(CurveTable)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}

void UCurveTableDerived::AddRow(FName Name, FRealCurve* Curve) {
	RowMap.Add(Name, Curve);
}

void UCurveTableDerived::ChangeTableMode(ECurveTableMode Mode) {
	CurveTableMode = Mode;
}
