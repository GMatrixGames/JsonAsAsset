// Copyright JAA Contributors 2023-2024

#pragma once

#include "UObject/StructOnScope.h"
#include "Importer.h"

class UDataTableImporter : public IImporter {
public:
	using FTableRowMap = TMap<FName, TSharedPtr<class FStructOnScope>>;

	UDataTableImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
	}

	virtual bool ImportData() override;
};
