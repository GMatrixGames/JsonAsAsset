// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialImporter.h"

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3) || ENGINE_MAJOR_VERSION == 4
#include "Materials/Material.h"
#else
#include "MaterialDomain.h"
#endif
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

		FString ShadingModelString;
		if (JsonObject->GetObjectField("Properties")->TryGetStringField("ShadingModel", ShadingModelString)) {
			EMaterialShadingModel ShadingModel = MSM_DefaultLit;

			if (ShadingModelString.EndsWith("MSM_Unlit")) ShadingModel = MSM_Unlit;
			else if (ShadingModelString.EndsWith("MSM_Subsurface")) ShadingModel = MSM_Subsurface;
			else if (ShadingModelString.EndsWith("MSM_PreintegratedSkin")) ShadingModel = MSM_PreintegratedSkin;
			else if (ShadingModelString.EndsWith("MSM_ClearCoat")) ShadingModel = MSM_ClearCoat;
			else if (ShadingModelString.EndsWith("MSM_SubsurfaceProfile")) ShadingModel = MSM_SubsurfaceProfile;
			else if (ShadingModelString.EndsWith("MSM_TwoSidedFoliage")) ShadingModel = MSM_TwoSidedFoliage;
			else if (ShadingModelString.EndsWith("MSM_Hair")) ShadingModel = MSM_Hair;
			else if (ShadingModelString.EndsWith("MSM_Cloth")) ShadingModel = MSM_Cloth;
			else if (ShadingModelString.EndsWith("MSM_Eye")) ShadingModel = MSM_Eye;
			else if (ShadingModelString.EndsWith("MSM_SingleLayerWater")) ShadingModel = MSM_SingleLayerWater;
			else if (ShadingModelString.EndsWith("MSM_ThinTranslucent")) ShadingModel = MSM_ThinTranslucent;
			else if (ShadingModelString.EndsWith("MSM_FromMaterialExpression")) ShadingModel = MSM_FromMaterialExpression;

			Material->SetShadingModel(ShadingModel);

			const TSharedPtr<FJsonObject>* ShadingModelsPtr;
			if (JsonObject->GetObjectField("Properties")->TryGetObjectField("ShadingModels", ShadingModelsPtr)) {
				int ShadingModelField;
				if (ShadingModelsPtr->Get()->TryGetNumberField("ShadingModelField", ShadingModelField)) Material->GetShadingModels().SetShadingModelField(ShadingModelField);
			}
		}

		bool bEnableResponsiveAA;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bEnableResponsiveAA", bEnableResponsiveAA)) Material->bEnableResponsiveAA = bEnableResponsiveAA;
		bool bUsedWithSkeletalMesh;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithSkeletalMesh", bUsedWithSkeletalMesh)) Material->bUsedWithSkeletalMesh = bUsedWithSkeletalMesh;
		bool bUsedWithParticleSprites;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithParticleSprites", bUsedWithParticleSprites)) Material->bUsedWithParticleSprites = bUsedWithParticleSprites;

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(Material)) return false;

		// Clear any default expressions the engine adds (ex: Result)
		Material->GetExpressionCollection().Empty();

		// Define editor only data from the JSON
		TMap<FName, FImportData> Exports;
		TArray<FName> ExpressionNames;
		TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), Exports, ExpressionNames)->GetObjectField("Properties");

		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// const TArray<TSharedPtr<FJsonValue>>* StringExpressions;
		// if (StringExpressionCollection->TryGetArrayField("Expressions", StringExpressions)) {
		// 	for (const TSharedPtr<FJsonValue> Expression : *StringExpressions) {
		// 		if (Expression->IsNull()) continue;
		// 		ExpressionNames.Add(GetExportNameOfSubobject(Expression->AsObject()->GetStringField("ObjectName")));
		// 	}
		// }

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = CreateExpressions(Material, ExpressionNames, Exports);

		const TSharedPtr<FJsonObject>* EmissiveColorPtr;
		if (EdProps->TryGetObjectField("EmissiveColor", EmissiveColorPtr) && EmissiveColorPtr != nullptr) {
			FJsonObject* EmissiveColorObject = EmissiveColorPtr->Get();
			FName EmissiveColorExpressionName = GetExpressionName(EmissiveColorObject);

			if (CreatedExpressionMap.Contains(EmissiveColorExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(EmissiveColorObject, *CreatedExpressionMap.Find(EmissiveColorExpressionName), "Color");
				FColorMaterialInput* EmissiveColor = reinterpret_cast<FColorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->EmissiveColor = *EmissiveColor;
			}
		}

		const TSharedPtr<FJsonObject>* OpacityPtr;
		if (EdProps->TryGetObjectField("Opacity", OpacityPtr) && OpacityPtr != nullptr) {
			FJsonObject* OpacityObject = OpacityPtr->Get();
			FName OpacityExpressionName = GetExpressionName(OpacityObject);

			if (CreatedExpressionMap.Contains(OpacityExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(OpacityObject, *CreatedExpressionMap.Find(OpacityExpressionName), "Scalar");
				FScalarMaterialInput* Opacity = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Opacity = *Opacity;
			}
		}

		const TSharedPtr<FJsonObject>* OpacityMaskPtr;
		if (EdProps->TryGetObjectField("OpacityMask", OpacityMaskPtr) && OpacityMaskPtr != nullptr) {
			FJsonObject* OpacityMaskObject = OpacityMaskPtr->Get();
			FName OpacityMaskExpressionName = GetExpressionName(OpacityMaskObject);

			if (CreatedExpressionMap.Contains(OpacityMaskExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(OpacityMaskObject, *CreatedExpressionMap.Find(OpacityMaskExpressionName), "Scalar");
				FScalarMaterialInput* OpacityMask = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->OpacityMask = *OpacityMask;
			}
		}

		const TSharedPtr<FJsonObject>* WorldPositionOffsetPtr;
		if (EdProps->TryGetObjectField("WorldPositionOffset", WorldPositionOffsetPtr) && WorldPositionOffsetPtr != nullptr) {
			FJsonObject* WorldPositionOffsetObject = WorldPositionOffsetPtr->Get();
			FName WorldPositionOffsetExpressionName = GetExpressionName(WorldPositionOffsetObject);

			if (CreatedExpressionMap.Contains(WorldPositionOffsetExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(WorldPositionOffsetObject, *CreatedExpressionMap.Find(WorldPositionOffsetExpressionName), "Vector");
				FVectorMaterialInput* WorldPositionOffset = reinterpret_cast<FVectorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->WorldPositionOffset = *WorldPositionOffset;
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

		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			FString ExType = Object->GetStringField("Type");
			FString Name = Object->GetStringField("Name");

			if (ExType == "MaterialGraph" && Name != "MaterialGraph_0") {
				TSharedPtr<FJsonObject> Properties = Object->GetObjectField("Properties");

				UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
				UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), OutermostPkg, *(FileName + "_SubGraph_" + Name.Replace(TEXT(" "), TEXT(""))), RF_Standalone | RF_Public, nullptr, GWarn));
				
				// Clear any default expressions the engine adds (ex: Result)
				MaterialFunction->GetExpressionCollection().Empty();

				TMap<FName, FImportData> NewExports;
				TArray<FName> NewExpressionNames;
				for (const TSharedPtr<FJsonValue> NewValue : AllJsonObjects) {
					TSharedPtr<FJsonObject> NewObject = TSharedPtr(NewValue->AsObject());

					FString OldExType = NewObject->GetStringField("Type");
					FString Outer = NewObject->GetStringField("Outer");

					if (Outer == Name) {
						const TSharedPtr<FJsonObject>* MaterialExpression;

						if (!NewObject->GetObjectField("Properties")->TryGetObjectField("MaterialExpression", MaterialExpression)) {
							continue;
						}

						FName NewName = GetExportNameOfSubobject(MaterialExpression->Get()->GetStringField("ObjectName"));

						FImportData* NewType = Exports.Find(NewName);
						FString NewExType = NewType->Json->GetStringField("Type");

						NewExpressionNames.Add(NewName);
						NewExports.Add(NewName, FImportData(NewExType, NewType->Json));
					}

					continue;
				}

				TMap<FName, UMaterialExpression*> NewCreatedExpressionMap = CreateExpressions(MaterialFunction, NewExpressionNames, NewExports);
				AddExpressions(MaterialFunction, NewExpressionNames, NewExports, NewCreatedExpressionMap);

				HandleAssetCreation(MaterialFunction);
				continue;
			}
		}

		// Iterate through all the expression names
		AddExpressions(Material, ExpressionNames, Exports, CreatedExpressionMap, true);

		AddComments(Material, StringExpressionCollection, Exports);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
