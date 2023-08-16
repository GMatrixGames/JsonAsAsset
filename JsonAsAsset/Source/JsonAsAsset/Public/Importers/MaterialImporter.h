// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Utilities/EditorGraph/MaterialGraph_Interface.h"

class UMaterialImporter : public UMaterialGraph_Interface {
public:
	UMaterialImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		UMaterialGraph_Interface(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;

	// Subgraph Functions
	TArray<TSharedPtr<FJsonValue>> FilterGraphNodesBySubgraphExpression(const FString& Outer);
};
