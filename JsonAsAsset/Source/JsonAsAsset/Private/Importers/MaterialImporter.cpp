// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialImporter.h"

#include "Materials/Material.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Dom/JsonObject.h"
#include "Factories/MaterialFactoryNew.h"
#include "Utilities/MathUtilities.h"
#include "MaterialEditor/Private/MaterialEditor.h"
#include "MaterialGraph/MaterialGraph.h"

#include "Settings/JsonAsAssetSettings.h"

bool UMaterialImporter::ImportData() {
	try {
		// Create Material Factory (factory automatically creates the M)
		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		TSharedPtr<FJsonObject> SerializerProperties = TSharedPtr<FJsonObject>(Properties);
		if (SerializerProperties->HasField("ShadingModel")) // ShadingModel set manually
			SerializerProperties->RemoveField("ShadingModel");

		GetObjectSerializer()->DeserializeObjectProperties(SerializerProperties, Material);
		FString ShadingModel;
		
		if (Properties->TryGetStringField("ShadingModel", ShadingModel) && ShadingModel != "EMaterialShadingModel::MSM_FromMaterialExpression")
			Material->SetShadingModel(static_cast<EMaterialShadingModel>(StaticEnum<EMaterialShadingModel>()->GetValueByNameString(ShadingModel)));

		// Clear any default expressions the engine adds (ex: Result)
		Material->Expressions.Empty();

		// Define editor only data from the JSON
		TMap<FName, FImportData> Exports;
		TArray<FName> ExpressionNames;
		TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), Material->GetName(), Exports, ExpressionNames, false)->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = ConstructExpressions(Material, Material->GetName(), ExpressionNames, Exports);
		const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

		// Iterate through all the expression names
		PropagateExpressions(Material, ExpressionNames, Exports, CreatedExpressionMap, true);
		MaterialGraphNode_ConstructComments(Material, StringExpressionCollection, Exports);

		if (!Settings->bSkipResultNodeConnection) {
			TArray<FString> IgnoredProperties = TArray<FString> {
				"ParameterGroupData",
				"ExpressionCollection",
				"CustomizedUVs"
			};

			const TSharedPtr<FJsonObject> RawConnectionData = TSharedPtr<FJsonObject>(EdProps);
			for (FString Property : IgnoredProperties) {
				if (RawConnectionData->HasField(Property))
					RawConnectionData->RemoveField(Property);
			}

			// Connect all pins using deserializer
			GetObjectSerializer()->DeserializeObjectProperties(RawConnectionData, Material);

			const TArray<TSharedPtr<FJsonValue>>* InputsPtr;

			// CustomizedUVs defined here
			if (EdProps->TryGetArrayField("CustomizedUVs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Material->CustomizedUVs[i] = *reinterpret_cast<FVector2MaterialInput*>(&Input);
					}
					i++;
				}
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* StringParameterGroupData;
		if (EdProps->TryGetArrayField("ParameterGroupData", StringParameterGroupData)) {
			TArray<FParameterGroupData> ParameterGroupData;

			for (const TSharedPtr<FJsonValue> ParameterGroupDataObject : *StringParameterGroupData) {
				if (ParameterGroupDataObject->IsNull()) continue;
				FParameterGroupData GroupData;

				FString GroupName;
				if (ParameterGroupDataObject->AsObject()->TryGetStringField("GroupName", GroupName)) GroupData.GroupName = GroupName;
				int GroupSortPriority;
				if (ParameterGroupDataObject->AsObject()->TryGetNumberField("GroupSortPriority", GroupSortPriority)) GroupData.GroupSortPriority = GroupSortPriority;

				ParameterGroupData.Add(GroupData);
			}

			Material->ParameterGroupData = ParameterGroupData;
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(Material)) return false;

		SavePackage();
		Material->PostEditChange();
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}

// Filter out Material Graph Nodes
// by checking their subgraph expression (composite)
TArray<TSharedPtr<FJsonValue>> UMaterialImporter::FilterGraphNodesBySubgraphExpression(const FString& Outer) {
	TArray<TSharedPtr<FJsonValue>> ReturnValue = TArray<TSharedPtr<FJsonValue>>();

	/*
	* How this works:
	* 1. Find a Material Graph Node
	* 2. Get the Material Expression
	* 3. Compare SubgraphExpression to the one provided
	*    to the function
	*/
	for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
		const TSharedPtr<FJsonObject> ValueObject = TSharedPtr<FJsonObject>(Value->AsObject());
		const TSharedPtr<FJsonObject> Properties = TSharedPtr<FJsonObject>(ValueObject->GetObjectField("Properties"));

		const TSharedPtr<FJsonObject>* MaterialExpression;
		if (Properties->TryGetObjectField("MaterialExpression", MaterialExpression)) {
			TSharedPtr<FJsonValue> ExpValue = GetExportByObjectPath(*MaterialExpression);
			TSharedPtr<FJsonObject> Expression = TSharedPtr<FJsonObject>(ExpValue->AsObject());
			const TSharedPtr<FJsonObject> _Properties = TSharedPtr<FJsonObject>(Expression->GetObjectField("Properties"));

			const TSharedPtr<FJsonObject>* _SubgraphExpression;
			if (_Properties->TryGetObjectField("SubgraphExpression", _SubgraphExpression)) {
				if (Outer == GetExportNameOfSubobject(_SubgraphExpression->Get()->GetStringField("ObjectName")).ToString()) {
					ReturnValue.Add(ExpValue);
				}
			}
		}
	}

	return ReturnValue;
}