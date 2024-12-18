// Copyright JAA Contributors 2024-2025

#pragma once

#include "./Importer.h"

/**
* Basic template importer using Asset Class.
*/
template <typename AssetType>
class TemplatedImporter : public IImporter {
public:
	const UClass* AssetClass;

	TemplatedImporter(UClass* AssetClass, const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects) :
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects),
		AssetClass(AssetClass) {}

	virtual bool ImportData() override;
};