// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Materials/MaterialExpressionComposite.h"
#include "Importer.h"

class UMaterialFunctionImporter : public IImporter {
public:
	UMaterialFunctionImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;

protected:
	struct FImportData {
		FImportData(const FName Type, const TSharedPtr<FJsonObject>& Json) {
			this->Type = Type;
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, const TSharedPtr<FJsonObject>& Json) {
			this->Type = FName(Type);
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, FJsonObject* Json) {
			this->Type = FName(Type);
			this->Json = Json;
		}

		FName Type;
		FJsonObject* Json;
	};
	
	TSharedPtr<FJsonObject> FindEditorOnlyData(const FString& Type, TMap<FName, FImportData>& OutExports);

	TMap<FName, UMaterialExpression*> CreateExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports);

	// Material Graphs (little nodes that contain a graph)
	bool HandleMaterialGraph(UObject* Parent, TSharedPtr<FJsonObject> JsonProperties, TMap<FName, FImportData> Exports);

	void AddExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap);
	
	void AddComments(UObject* Parent, TSharedPtr<FJsonObject> Json, TMap<FName, FImportData>& Exports);
	
	void AddGenerics(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json);

	UMaterialExpression* CreateEmptyExpression(UObject* Parent, FName Name, FName Type) const;

	FExpressionInput PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression);
	FExpressionOutput PopulateExpressionOutput(const FJsonObject* JsonProperties);

	FName GetExpressionName(const FJsonObject* JsonProperties);

	FFunctionExpressionOutput PopulateFuncExpressionOutput(const TSharedPtr<FJsonObject>& JsonProperties);
	FFunctionExpressionInput PopulateFuncExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap);
};
