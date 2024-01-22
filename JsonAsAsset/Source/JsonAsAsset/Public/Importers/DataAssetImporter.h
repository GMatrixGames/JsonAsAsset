// Copyright JAA Contributors 2023-2024

#pragma once
#include "Importer.h"

class UDataAssetImporter : public IImporter {
public:
	const UClass* DataAssetClass;

	UDataAssetImporter(UClass* DataClass, const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects) :
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
		this->DataAssetClass = DataClass;
	}

	virtual bool ImportData() override;
};
