// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

class USkeletonImporter : public IImporter {
public:
	USkeletonImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, TArray<TSharedPtr<FJsonValue>>& JsonObjects):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, JsonObjects) {
	}

	virtual bool ImportData() override;
};
