// Copyright Epic Games, Inc. All Rights Reserved.

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
			UScriptStruct* InStruct = CurveTableMode == ECurveTableMode::SimpleCurves ?
				FSimpleCurve::StaticStruct()
				: FRichCurve::StaticStruct();

			TSharedPtr<FStructOnScope> Struct = MakeShareable(new FStructOnScope(InStruct));
			PropertySerializer->DeserializeStruct(InStruct, CurveData.ToSharedRef(), Struct->GetStructMemory());

			// Add Row using Derived Reference
			DerivedCurveTable->AddRow(FName(*Pair.Key), reinterpret_cast<FRealCurve*>(&Struct));

			// Update Curve Table
			CurveTable->OnCurveTableChanged().Broadcast();
			CurveTable->Modify(true);
		}

		// Handle edit changes, and add it to the content browser
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
