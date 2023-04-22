﻿// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

class USoundConcurrencyImporter : public IImporter {
public:
	USoundConcurrencyImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
	}

	virtual bool ImportData() override;
};
