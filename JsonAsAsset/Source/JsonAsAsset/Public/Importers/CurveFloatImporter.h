// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

class UCurveFloatImporter : public IImporter
{
public:
	UCurveFloatImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, JsonObject, Package, OutermostPkg)
	{
	}

	virtual bool ImportData() override;
};
