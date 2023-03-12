// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/DataTableImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"

bool UDataTableImporter::ImportData() {
	try {
		/* Properties of the object
		TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");
		
		UPackage* Package = CreateAssetPackage(Name, OutFileNames);
		UDataTable* DataTable = NewObject<UDataTable>(Package, UDataTable::StaticClass(), *Name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

		FString ClassName;

		Properties->GetObjectField("RowStruct").Get()->GetStringField("ObjectName").Split(" ", nullptr, &ClassName);
		FString ClassModule = Properties->GetObjectField("RowStruct").Get()->GetStringField("ObjectPath");
		UScriptStruct* ScriptStruct = FindObject<UScriptStruct>(ANY_PACKAGE, *(ClassModule + "." + ClassName));

		DataTable->RowStruct = ScriptStruct;

		for (auto pair = DataObject->GetObjectField("Rows")->Values.CreateConstIterator(); pair; ++pair) {
			const FString PairName = (*pair).Key;
			TSharedPtr<FJsonValue> PairValue = (*pair).Value;

			FTableRowBase* CurRow = reinterpret_cast<FTableRowBase*>(ScriptStruct);
			DataTable->AddRow(FName(*PairName), *CurRow);
		}

		FAssetRegistryModule::AssetCreated(DataTable);
		DataTable->MarkPackageDirty();
		Package->SetDirtyFlag(true);
		DataTable->PostEditChange();
		DataTable->AddToRoot();
		*/

		UDataTable* Asset = Cast<UDataTable>(FAssetUtilities::GetSelectedAsset());

		for (TMap<FName, uint8*>::TConstIterator RowMapIter(Asset->GetRowMap().CreateConstIterator()); RowMapIter; ++RowMapIter) {
			if (ensure(RowMapIter.Value() != nullptr)) {
				const uint8* RowDataPtr = RowMapIter.Value();

				const UScriptStruct* CurrentStruct = reinterpret_cast<UScriptStruct*>(*RowDataPtr);

				FText DialogText = FText::FromString(CurrentStruct->GetName());
				FMessageDialog::Open(EAppMsgType::Ok, DialogText);

				//UE_LOG(LogTemp, Warning, TEXT("Row Name: %s ; ScriptStructName:- %s"), RowMapIter.Key(), CurrentStruct->GetName());
			}
		}
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
