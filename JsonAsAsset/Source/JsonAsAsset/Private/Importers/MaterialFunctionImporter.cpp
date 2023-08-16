// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"
#include "Materials/MaterialFunction.h"
#include "Factories/MaterialFunctionFactoryNew.h"

bool UMaterialFunctionImporter::ImportData() {
	try {
		// Create Material Function Factory (factory automatically creates the MF)
		UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), Cast<UObject>(OutermostPkg), *FileName, RF_Standalone | RF_Public, nullptr, GWarn));
		MaterialFunction->FunctionExpressions.Empty();

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(MaterialFunction)) return false;

		MaterialFunction->StateId = CreateGUID(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));
		
		FString Description;
		bool bExposeToLibrary;
		bool bPrefixParameterNames;

		// Misc properties
		if (JsonObject->GetObjectField("Properties")->TryGetStringField("Description", Description)) MaterialFunction->Description = Description;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bExposeToLibrary", bExposeToLibrary)) MaterialFunction->bExposeToLibrary = bExposeToLibrary;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bPrefixParameterNames", bPrefixParameterNames)) MaterialFunction->bPrefixParameterNames = bPrefixParameterNames;

		// Define editor only data from the JSON
		TMap<FName, FImportData> Exports;
		TArray<FName> ExpressionNames;
		const TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), MaterialFunction->GetName(), Exports, ExpressionNames, false)->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = ConstructExpressions(MaterialFunction, MaterialFunction->GetName(), ExpressionNames, Exports);

		// Iterate through all the expression names
		PropagateExpressions(MaterialFunction, ExpressionNames, Exports, CreatedExpressionMap);
		MaterialGraphNode_ConstructComments(MaterialFunction, StringExpressionCollection, Exports);

	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception))
		return false;
	}

	return true;
}
