// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"
#include "Materials/MaterialExpressionComposite.h"
#include "MaterialFunctionImporter.h"

class UMaterialImporter : public UMaterialFunctionImporter {
public:
	UMaterialImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		UMaterialFunctionImporter(FileName, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	void ComposeExpressionPinBase(UMaterialExpressionPinBase* Pin, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, const TSharedPtr<FJsonObject>& _JsonObject, TMap<FName, FImportData>& Exports);
	virtual bool ImportData() override;
};
