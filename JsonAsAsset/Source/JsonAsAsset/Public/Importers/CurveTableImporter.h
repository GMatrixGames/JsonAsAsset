// Copyright JAA Contributors 2023-2024

#pragma once

#include "Importer.h"
#include "Engine/CurveTable.h"

class UCurveTableDerived : public UCurveTable {
public:
	void AddRow(FName Name, FRealCurve* Curve);
	void ChangeTableMode(ECurveTableMode Mode);
};

class UCurveTableImporter : public IImporter {
public:
	UCurveTableImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
	}

	virtual bool ImportData() override;
};
