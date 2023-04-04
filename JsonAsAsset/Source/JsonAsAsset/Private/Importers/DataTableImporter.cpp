// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/DataTableImporter.h"

#include "Dom/JsonObject.h"
#include "Utilities/AssetUtilities.h"

bool UDataTableImporter::ImportData() {
	try {
		// Used to set RowStruct
		TSharedPtr<FJsonObject> AssetData = JsonObject->GetObjectField("Properties");
		UDataTable* NewDataTable = NewObject<UDataTable>(Package, UDataTable::StaticClass(), *FileName, RF_Public | RF_Standalone);
		HandleAssetCreation(NewDataTable);

	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
