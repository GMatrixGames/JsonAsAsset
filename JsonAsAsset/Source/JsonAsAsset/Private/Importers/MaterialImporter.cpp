// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialImporter.h"

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3) || ENGINE_MAJOR_VERSION == 4
#include "Materials/Material.h"
#else
#include "MaterialDomain.h"
#endif
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Dom/JsonObject.h"
#include "Factories/MaterialFactoryNew.h"
#include "Utilities/MathUtilities.h"
#include "MaterialEditor/Private/MaterialEditor.h"
#include "MaterialGraph/MaterialGraph.h"
#include <Editor/UnrealEd/Classes/MaterialGraph/MaterialGraphNode_Composite.h>
#include <Editor/UnrealEd/Classes/MaterialGraph/MaterialGraphSchema.h>

#include "Settings/JsonAsAssetSettings.h"

void UMaterialImporter::ComposeExpressionPinBase(UMaterialExpressionPinBase* Pin, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, const TSharedPtr<FJsonObject>& _JsonObject, TMap<FName, FImportData>& Exports) {
	FJsonObject* Expression = (Exports.Find(GetExportNameOfSubobject(_JsonObject->GetStringField("ObjectName")))->Json)->GetObjectField("Properties").Get();

	Pin->GraphNode->NodePosX = Expression->GetNumberField("MaterialExpressionEditorX");
	Pin->GraphNode->NodePosY = Expression->GetNumberField("MaterialExpressionEditorY");
	Pin->MaterialExpressionEditorX = Expression->GetNumberField("MaterialExpressionEditorX");
	Pin->MaterialExpressionEditorY = Expression->GetNumberField("MaterialExpressionEditorY");

	FString MaterialExpressionGuid;
	if (Expression->TryGetStringField("MaterialExpressionGuid", MaterialExpressionGuid)) Pin->MaterialExpressionGuid = FGuid(MaterialExpressionGuid);

	const TArray<TSharedPtr<FJsonValue>>* ReroutePins;
	if (Expression->TryGetArrayField("ReroutePins", ReroutePins)) {
		for (const TSharedPtr<FJsonValue> ReroutePin : *ReroutePins) {
			if (ReroutePin->IsNull()) continue;
			TSharedPtr<FJsonObject> ReroutePinObject = ReroutePin->AsObject();
			TSharedPtr<FJsonObject> RerouteObj = GetExportByObjectPath(ReroutePinObject->GetObjectField("Expression"))->AsObject();
			UMaterialExpressionReroute* RerouteExpression = Cast<UMaterialExpressionReroute>(*CreatedExpressionMap.Find(FName(RerouteObj->GetStringField("Name"))));

			// Add reroute to pin
			Pin->ReroutePins.Add(FCompositeReroute(FName(ReroutePinObject->GetStringField("Name")), RerouteExpression));
		}
	}

	// Set Pin Direction
	FString PinDirection;
	if (Expression->TryGetStringField("PinDirection", PinDirection)) {
		EEdGraphPinDirection Enum_PinDirection = EGPD_Input;

		if (PinDirection.EndsWith("EGPD_Input")) Enum_PinDirection = EGPD_Input;
		else if (PinDirection.EndsWith("EGPD_Output")) Enum_PinDirection = EGPD_Output;

		Pin->PinDirection = Enum_PinDirection;
	}

	const TArray<TSharedPtr<FJsonValue>>* OutputsPtr;
	if (Expression->TryGetArrayField("Outputs", OutputsPtr)) {
		TArray<FExpressionOutput> Outputs;
		for (const TSharedPtr<FJsonValue> OutputValue : *OutputsPtr) {
			TSharedPtr<FJsonObject> OutputObject = OutputValue->AsObject();
			Outputs.Add(PopulateExpressionOutput(OutputObject.Get()));
		}

		Pin->Outputs = Outputs;
	}

	Pin->Modify();
}

bool UMaterialImporter::ImportData() {
	try {
		// Create Material Factory (factory automatically creates the M)
		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		// Clear any default expressions the engine adds (ex: Result)
		Material->GetExpressionCollection().Empty();

		// Define editor only data from the JSON
		TMap<FName, FImportData> Exports;
		TArray<FName> ExpressionNames;
		TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), Material->GetName(), Exports, ExpressionNames, false)->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = ConstructExpressions(Material, Material->GetName(), ExpressionNames, Exports);
		const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

		// Missing Material Data
		if (Exports.IsEmpty()) {
			FNotificationInfo Info = FNotificationInfo(FText::FromString("Material Data Missing (" + FileName + ")"));
			Info.ExpireDuration = 7.0f;
			Info.bUseLargeFont = true;
			Info.bUseSuccessFailIcons = true;
			Info.WidthOverride = FOptionalSize(350);
			Info.SubText = FText::FromString(FString("Please use the correct FModel provided in the JsonAsAsset server."));

			TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
			NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);

			return false;
		}

		// Iterate through all the expression names
		PropagateExpressions(Material, ExpressionNames, Exports, CreatedExpressionMap, true);
		MaterialGraphNode_ConstructComments(Material, StringExpressionCollection, Exports);

		if (!Settings->bSkipResultNodeConnection) {
			TArray<FString> IgnoredProperties = TArray<FString> {
				"ParameterGroupData",
				"ExpressionCollection",
				"CustomizedUVs"
			};

			const TSharedPtr<FJsonObject> RawConnectionData = TSharedPtr(EdProps);
			for (FString Property : IgnoredProperties) {
				if (RawConnectionData->HasField(Property))
					RawConnectionData->RemoveField(Property);
			}

			// Connect all pins using deserializer
			GetObjectSerializer()->DeserializeObjectProperties(RawConnectionData, Material->GetEditorOnlyData());

			// CustomizedUVs defined here
			if (const TArray<TSharedPtr<FJsonValue>>* InputsPtr; EdProps->TryGetArrayField("CustomizedUVs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Material->GetEditorOnlyData()->CustomizedUVs[i] = *reinterpret_cast<FVector2MaterialInput*>(&Input);
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

			Material->GetEditorOnlyData()->ParameterGroupData = ParameterGroupData;
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(Material)) return false;

		bool bEditorGraphOpen = false;
		FMaterialEditor* AssetEditorInstance = nullptr;

		// Handle Material Graphs
		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			FString ExType = Object->GetStringField("Type");
			FString Name = Object->GetStringField("Name");

			if (ExType == "MaterialGraph" && Name != "MaterialGraph_0") {
				TSharedPtr<FJsonObject> GraphProperties = Object->GetObjectField("Properties");
				TSharedPtr<FJsonObject> SubgraphExpression;

				FString SubgraphExpressionName;

				if (!bEditorGraphOpen) {
					// Create Editor
					UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
					AssetEditorInstance = reinterpret_cast<FMaterialEditor*>(AssetEditorSubsystem->OpenEditorForAsset(Material) ? AssetEditorSubsystem->FindEditorForAsset(Material, true) : nullptr);

					bEditorGraphOpen = true;
				}

				// Set SubgraphExpression
				const TSharedPtr<FJsonObject>* SubgraphExpressionPtr = nullptr;
				if (GraphProperties->TryGetObjectField("SubgraphExpression", SubgraphExpressionPtr) && SubgraphExpressionPtr != nullptr) {
					FJsonObject* SubgraphExpressionObject = SubgraphExpressionPtr->Get();
					FName ExportName = GetExportNameOfSubobject(SubgraphExpressionObject->GetStringField("ObjectName"));

					SubgraphExpressionName = ExportName.ToString();
					FImportData Export = *Exports.Find(ExportName);
					SubgraphExpression = Export.Json->GetObjectField("Properties");
				}

				// Find Material Graph
				UMaterialGraph* MaterialGraph = AssetEditorInstance->Material->MaterialGraph;
				if (MaterialGraph == nullptr) {
					UE_LOG(LogJson, Log, TEXT("The material graph is not valid!"));
				}

				MaterialGraph->Modify();

				// Create the composite node that will serve as the gateway into the subgraph
				UMaterialGraphNode_Composite* GatewayNode = nullptr;
				{
					GatewayNode = Cast<UMaterialGraphNode_Composite>(FMaterialGraphSchemaAction_NewComposite::SpawnNode(MaterialGraph, FVector2D(SubgraphExpression->GetNumberField("MaterialExpressionEditorX"), SubgraphExpression->GetNumberField("MaterialExpressionEditorY"))));
					GatewayNode->bCanRenameNode = true;
					check(GatewayNode);
				}

				UMaterialGraph* DestinationGraph = Cast<UMaterialGraph>(GatewayNode->BoundGraph);
				UMaterialExpressionComposite* CompositeExpression = CastChecked<UMaterialExpressionComposite>(GatewayNode->MaterialExpression);
				{
					CompositeExpression->Material = Material;
					CompositeExpression->SubgraphName = Name;

					MaterialGraphNode_ExpressionWrapper(Material, CompositeExpression, SubgraphExpression);
				}

				// Create notification
				FNotificationInfo Info = FNotificationInfo(FText::FromString("Material Graph imported incomplete"));
				Info.ExpireDuration = 2.0f;
				Info.bUseLargeFont = true;
				Info.bUseSuccessFailIcons = true;
				Info.WidthOverride = FOptionalSize(350);
				Info.SubText = FText::FromString(FString("Material"));

				// Set icon as successful
				TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
				NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);

				DestinationGraph->Rename(*CompositeExpression->SubgraphName);
				DestinationGraph->Material = MaterialGraph->Material;

				// Add Sub-Graph Nodes
				{
					TArray<TSharedPtr<FJsonValue>> MaterialGraphNodes = FilterGraphNodesBySubgraphExpression(SubgraphExpressionName);
					TMap<FName, FImportData> SubGraphExports;
					TMap<FName, UMaterialExpression*> SubgraphExpressionMapping;
					TArray<FName> SubGraphExpressionNames;

					// Go through each expression
					for (const TSharedPtr<FJsonValue> _GraphNode : MaterialGraphNodes) {
						const TSharedPtr<FJsonObject> MaterialGraphObject = TSharedPtr(_GraphNode->AsObject());

						FString GraphNode_Type = MaterialGraphObject->GetStringField("Type");
						FString GraphNode_Name = MaterialGraphObject->GetStringField("Name");

						FName GraphNodeNameName = FName(GraphNode_Name);

						TSharedPtr<FJsonObject>* SharedGraphObject = new TSharedPtr<FJsonObject>(MaterialGraphObject);

						UMaterialExpression* Ex = CreateEmptyExpression(MaterialGraph->Material, GraphNodeNameName, FName(GraphNode_Type), SharedGraphObject->Get());
						if (Ex == nullptr)
							continue;

						Ex->SubgraphExpression = CompositeExpression;
						Ex->Material = MaterialGraph->Material;

						SubGraphExpressionNames.Add(GraphNodeNameName);
						SubGraphExports.Add(GraphNodeNameName, FImportData(GraphNode_Type, MaterialGraph->Material->GetName(), MaterialGraphObject));
						SubgraphExpressionMapping.Add(GraphNodeNameName, Ex);
					}

					// Setup Input/Output Expressions
					{
						const TSharedPtr<FJsonObject>* InputExpressions;
						if (SubgraphExpression->TryGetObjectField("InputExpressions", InputExpressions)) {
							TSharedPtr<FJsonObject> InputExpression = TSharedPtr<FJsonObject>(InputExpressions->Get());

							ComposeExpressionPinBase(CompositeExpression->InputExpressions, CreatedExpressionMap, InputExpression, Exports);
						}

						const TSharedPtr<FJsonObject>* OutputExpressions;
						if (SubgraphExpression->TryGetObjectField("OutputExpressions", OutputExpressions)) {
							TSharedPtr<FJsonObject> OutputExpression = TSharedPtr<FJsonObject>(OutputExpressions->Get());

							ComposeExpressionPinBase(CompositeExpression->OutputExpressions, CreatedExpressionMap, OutputExpression, Exports);
						}
					}

					// Add all the expression properties
					PropagateExpressions(MaterialGraph->Material, SubGraphExpressionNames, Exports, SubgraphExpressionMapping, true);

					// All expressions (hopefully) have their properties
					// so now we just make a material graph node for each
					for (const TPair<FName, UMaterialExpression*>& pair : SubgraphExpressionMapping) {
						UMaterialExpression* Expression = pair.Value;
						UMaterialGraphNode* NewNode = DestinationGraph->AddExpression(Expression, false);

						const FGuid NewGuid = FGuid::NewGuid();
						NewNode->NodeGuid = NewGuid;

						NewNode->NodePosX = Expression->MaterialExpressionEditorX;
						NewNode->NodePosY = Expression->MaterialExpressionEditorY;
						NewNode->MaterialExpression = Expression;

						DestinationGraph->AddNode(NewNode);
						NewNode->ReconstructNode();
					}
				}

				CompositeExpression->InputExpressions->Material = MaterialGraph->Material;
				CompositeExpression->OutputExpressions->Material = MaterialGraph->Material;

				GatewayNode->ReconstructNode();
				Cast<UMaterialGraphNode>(CompositeExpression->InputExpressions->GraphNode)->ReconstructNode();
				Cast<UMaterialGraphNode>(CompositeExpression->OutputExpressions->GraphNode)->ReconstructNode();

				DestinationGraph->RebuildGraph();
				DestinationGraph->LinkMaterialExpressionsFromGraph();

				// Update Original Material
				AssetEditorInstance->UpdateMaterialAfterGraphChange();
			}
		}

		if (FString ShadingModel; Properties->TryGetStringField("ShadingModel", ShadingModel) && ShadingModel != "EMaterialShadingModel::MSM_FromMaterialExpression")
			Material->SetShadingModel(static_cast<EMaterialShadingModel>(StaticEnum<EMaterialShadingModel>()->GetValueByNameString(ShadingModel)));

		if (const TSharedPtr<FJsonObject>* ShadingModelsPtr; Properties->TryGetObjectField("ShadingModels", ShadingModelsPtr))
			if (int ShadingModelField; ShadingModelsPtr->Get()->TryGetNumberField("ShadingModelField", ShadingModelField))
				Material->GetShadingModels().SetShadingModelField(ShadingModelField);

		TSharedPtr<FJsonObject> SerializerProperties = TSharedPtr(Properties);
		if (SerializerProperties->HasField("ShadingModel")) // ShadingModel set manually
			SerializerProperties->RemoveField("ShadingModel");

		GetObjectSerializer()->DeserializeObjectProperties(SerializerProperties, Material);

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
		const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Value->AsObject());
		const TSharedPtr<FJsonObject> Properties = TSharedPtr(ValueObject->GetObjectField("Properties"));

		const TSharedPtr<FJsonObject>* MaterialExpression;
		if (Properties->TryGetObjectField("MaterialExpression", MaterialExpression)) {
			TSharedPtr<FJsonValue> ExpValue = GetExportByObjectPath(*MaterialExpression);
			TSharedPtr<FJsonObject> Expression = TSharedPtr(ExpValue->AsObject());
			const TSharedPtr<FJsonObject> _Properties = TSharedPtr(Expression->GetObjectField("Properties"));

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