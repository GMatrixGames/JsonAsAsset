// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"
#include "Factories/MaterialFunctionFactoryNew.h"

#include "UObject/SavePackage.h"

bool UMaterialFunctionImporter::ImportData() {
	try {
		// Create Material Function Factory (factory automatically creates the MF)
		UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));
		MaterialFunction->GetExpressionCollection().Empty();

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(MaterialFunction)) return false;

		MaterialFunction->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));
		
		// Misc properties
		if (FString Description; JsonObject->GetObjectField("Properties")->TryGetStringField("Description", Description)) MaterialFunction->Description = Description;
		if (bool bExposeToLibrary; JsonObject->GetObjectField("Properties")->TryGetBoolField("bExposeToLibrary", bExposeToLibrary)) MaterialFunction->bExposeToLibrary = bExposeToLibrary;
		if (bool bPrefixParameterNames; JsonObject->GetObjectField("Properties")->TryGetBoolField("bPrefixParameterNames", bPrefixParameterNames)) MaterialFunction->bPrefixParameterNames = bPrefixParameterNames;

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

		MaterialFunction->PreEditChange(NULL);
		MaterialFunction->PostEditChange();

		SavePackage();
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception))
		return false;
	}

	return true;
}
