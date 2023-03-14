// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialImporter.h"

#include "Materials/Material.h"
#include "Dom/JsonObject.h"
#include "Factories/MaterialFactoryNew.h"

bool UMaterialImporter::ImportData() {
	try {
		// Create Material Factory (factory automatically creates the M)
		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		Material->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));
		bool bCanMaskedBeAssumedOpaque;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bCanMaskedBeAssumedOpaque", bCanMaskedBeAssumedOpaque)) Material->bCanMaskedBeAssumedOpaque = bCanMaskedBeAssumedOpaque;
		FString MaterialDomainString;
		if (JsonObject->GetObjectField("Properties")->TryGetStringField("MaterialDomain", MaterialDomainString)) {
			EMaterialDomain MaterialDomain = MD_Surface;

			if (MaterialDomainString.EndsWith("MD_DeferredDecal")) MaterialDomain = MD_DeferredDecal;
			else if (MaterialDomainString.EndsWith("MD_LightFunction")) MaterialDomain = MD_LightFunction;
			else if (MaterialDomainString.EndsWith("MD_Volume")) MaterialDomain = MD_Volume;
			else if (MaterialDomainString.EndsWith("MD_PostProcess")) MaterialDomain = MD_PostProcess;
			else if (MaterialDomainString.EndsWith("MD_UI")) MaterialDomain = MD_UI;

			Material->MaterialDomain = MaterialDomain;
		}

		FString BlendModeString;
		if (JsonObject->GetObjectField("Properties")->TryGetStringField("BlendMode", BlendModeString)) {
			EBlendMode BlendMode = BLEND_Translucent;

			if (BlendModeString.EndsWith("BLEND_Masked")) BlendMode = BLEND_Masked;
			else if (BlendModeString.EndsWith("BLEND_Translucent")) BlendMode = BLEND_Translucent;
			else if (BlendModeString.EndsWith("BLEND_Additive")) BlendMode = BLEND_Additive;
			else if (BlendModeString.EndsWith("BLEND_Modulate")) BlendMode = BLEND_Modulate;
			else if (BlendModeString.EndsWith("BLEND_AlphaComposite")) BlendMode = BLEND_AlphaComposite;
			else if (BlendModeString.EndsWith("BLEND_AlphaHoldout")) BlendMode = BLEND_AlphaHoldout;

			Material->BlendMode = BlendMode;
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(Material)) return false;

		// Clear any default expressions the engine adds (ex: Result)
		Material->GetExpressionCollection().Empty();

		// Define editor only data from the JSON
		TMap<FName, FImportData> Exports;
		TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), Exports)->GetObjectField("Properties");

		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		TArray<FName> ExpressionNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressions;
		if (StringExpressionCollection->TryGetArrayField("Expressions", StringExpressions)) {
			for (const TSharedPtr<FJsonValue> Expression : *StringExpressions) {
				if (Expression->IsNull()) continue;
				ExpressionNames.Add(GetExportNameOfSubobject(Expression->AsObject()->GetStringField("ObjectName")));
			}
		}

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = CreateExpressions(Material, ExpressionNames, Exports);

		const TSharedPtr<FJsonObject>* EmissiveColorPtr;
		if (EdProps->TryGetObjectField("EmissiveColor", EmissiveColorPtr) && EmissiveColorPtr != nullptr) {
			FJsonObject* EmissiveColorObject = EmissiveColorPtr->Get();
			FName EmissiveColorExpressionName = GetExpressionName(EmissiveColorObject);

			if (CreatedExpressionMap.Contains(EmissiveColorExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(EmissiveColorObject, *CreatedExpressionMap.Find(EmissiveColorExpressionName));
				FColorMaterialInput* EmissiveColor = reinterpret_cast<FColorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->EmissiveColor = *EmissiveColor;
			}
		}

		const TSharedPtr<FJsonObject>* OpacityPtr;
		if (EdProps->TryGetObjectField("Opacity", OpacityPtr) && OpacityPtr != nullptr) {
			FJsonObject* OpacityObject = OpacityPtr->Get();
			FName OpacityExpressionName = GetExpressionName(OpacityObject);

			if (CreatedExpressionMap.Contains(OpacityExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(OpacityObject, *CreatedExpressionMap.Find(OpacityExpressionName));
				FScalarMaterialInput* Opacity = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Opacity = *Opacity;
			}
		}

		// Iterate through all the expression names
		AddExpressions(Material, ExpressionNames, Exports, CreatedExpressionMap);

		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			FString ExType = Object->GetStringField("Type");
			FString Name = Object->GetStringField("Name");

			if (ExType == "MaterialGraph" && Name != "MaterialGraph_0") {
				HandleMaterialGraph(Material, Object->GetObjectField("Properties"), Exports);
				continue;
			}
		}

		AddComments(Material, StringExpressionCollection, Exports);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
