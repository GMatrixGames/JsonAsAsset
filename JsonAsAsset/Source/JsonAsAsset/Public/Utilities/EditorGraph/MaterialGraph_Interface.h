// Copyright JAA Contributors 2023-2024

#pragma once

#include "Importers/Importer.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

//		Material Graph Handler
// Handles everything needed to create
//   a material graph. (ex: nodes)
class UMaterialGraph_Interface : public IImporter {
public:
	UMaterialGraph_Interface(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	// UMaterialExpression, Properties
	UPROPERTY()
		TMap<FString, FJsonObject*> MissingNodeClasses;

protected:
	inline static TArray<FString> IgnoredExpressions = {
		"MaterialExpressionComposite",
		"MaterialExpressionPinBase",
		"MaterialExpressionComment",
		"MaterialFunction",
		"Material"
	};

	struct FImportData {
		FImportData(const FName Type, const FName Outer, const TSharedPtr<FJsonObject>& Json) {
			this->Type = Type;
			this->Outer = Outer;
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, const FString& Outer, const TSharedPtr<FJsonObject>& Json) {
			this->Type = FName(Type);
			this->Outer = FName(Outer);
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, const FString& Outer, FJsonObject* Json) {
			this->Type = FName(Type);
			this->Outer = FName(Outer);
			this->Json = Json;
		}

		FName Type;
		FName Outer;
		FJsonObject* Json;
	};

	// Find's Material Only Data
	TSharedPtr<FJsonObject> FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FImportData>& OutExports, TArray<FName>& ExpressionNames, bool bFilterByOuter = true);

	//		Functions to Handle Expressions ------------
	void MaterialGraphNode_ExpressionWrapper(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json);
	void MaterialGraphNode_ConstructComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FImportData>& Exports);

	// Makes each expression with their class
	TMap<FName, UMaterialExpression*> ConstructExpressions(UObject* Parent, const FString& Outer, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports);
	UMaterialExpression* CreateEmptyExpression(UObject* Parent, FName Name, FName Type, FJsonObject* LocalizedObject);

	// Modifies Graph Nodes (copies over properties from FJsonObject)
	void PropagateExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, bool bCheckOuter = false, bool bSubgraph = false);
	// ----------------------------------------------------

	//		Functions to Handle Node Connections ------------
	FExpressionInput PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type = "Default");
	FExpressionOutput PopulateExpressionOutput(const FJsonObject* JsonProperties);

	FName GetExpressionName(const FJsonObject* JsonProperties, FString OverrideParameterName = "Expression");

	FExpressionInput CreateExpressionInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName);
	FMaterialAttributesInput CreateMaterialAttributesInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName);
};
