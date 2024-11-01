// Copyright JAA Contributors 2024-2025

#pragma once

#include "../Constructor/MaterialGraph.h"

#include "Materials/MaterialExpressionComposite.h"

class UMaterialImporter : public IMaterialGraph {
public:
	UMaterialImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IMaterialGraph(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;

	// Subgraph Functions
	void ComposeExpressionPinBase(UMaterialExpressionPinBase* Pin, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, const TSharedPtr<FJsonObject>& _JsonObject, TMap<FName, FImportData>& Exports);
	TArray<TSharedPtr<FJsonValue>> FilterGraphNodesBySubgraphExpression(const FString& Outer);
};
