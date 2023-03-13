// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

class UMaterialFunctionImporter : public IImporter {
public:
	UMaterialFunctionImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;

private:
	void AddGenerics(UMaterialFunction* MaterialFunction, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json);

	UMaterialExpression* CreateEmptyExpression(UMaterialFunction* Parent, FName Name, FName Type) const;

	FExpressionInput PopulateExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, UMaterialExpression* Expression);
};
