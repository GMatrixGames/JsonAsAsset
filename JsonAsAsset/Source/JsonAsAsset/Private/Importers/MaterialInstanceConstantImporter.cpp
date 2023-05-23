// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialInstanceConstantImporter.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Utilities/MathUtilities.h"

#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
#include "Materials/MaterialExpressionStaticComponentMaskParameter.h"

bool UMaterialInstanceConstantImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		UMaterialInstanceConstant* MaterialInstanceConstant = NewObject<UMaterialInstanceConstant>(Package, UMaterialInstanceConstant::StaticClass(), *FileName, RF_Public | RF_Standalone);
		HandleAssetCreation(MaterialInstanceConstant);

		TArray<TSharedPtr<FJsonObject>> EditorOnlyData;

		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			if (Object->GetStringField("Type") == "MaterialInstanceEditorOnlyData") {
				EditorOnlyData.Add(Object);
			}
		}

		const TSharedPtr<FJsonObject>* ParentPtr = nullptr;
		if (Properties->TryGetObjectField("Parent", ParentPtr) && ParentPtr != nullptr) {
			LoadObject(ParentPtr, MaterialInstanceConstant->Parent);
		}

		TArray<FScalarParameterValue> ScalarParameterValues;

		TArray<TSharedPtr<FJsonValue>> Scalars = Properties->GetArrayField("ScalarParameterValues");
		for (int32 i = 0; i < Scalars.Num(); i++) {
			TSharedPtr<FJsonObject> Scalar = Scalars[i]->AsObject();

			FScalarParameterValue Parameter;
			Parameter.ParameterValue = Scalar->GetNumberField("ParameterValue");
			Parameter.ExpressionGUID = FGuid(Scalar->GetStringField("ExpressionGUID"));

			TSharedPtr<FJsonObject> ParameterInfoJson = Scalar->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;

			ScalarParameterValues.Add(Parameter);
		}

		MaterialInstanceConstant->ScalarParameterValues = ScalarParameterValues;

		TArray<FVectorParameterValue> VectorParameterValues;

		TArray<TSharedPtr<FJsonValue>> Vectors = Properties->GetArrayField("VectorParameterValues");
		for (int32 i = 0; i < Vectors.Num(); i++) {
			TSharedPtr<FJsonObject> Vector = Vectors[i]->AsObject();

			FVectorParameterValue Parameter;
			Parameter.ExpressionGUID = FGuid(Vector->GetStringField("ExpressionGUID"));

			Parameter.ParameterValue = FMathUtilities::ObjectToLinearColor(Vector->GetObjectField("ParameterValue").Get());

			TSharedPtr<FJsonObject> ParameterInfoJson = Vector->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;

			VectorParameterValues.Add(Parameter);
		}

		MaterialInstanceConstant->VectorParameterValues = VectorParameterValues;

		TArray<FTextureParameterValue> TextureParameterValues;

		TArray<TSharedPtr<FJsonValue>> Textures = Properties->GetArrayField("TextureParameterValues");
		for (int32 i = 0; i < Textures.Num(); i++) {
			TSharedPtr<FJsonObject> Texture = Textures[i]->AsObject();

			FTextureParameterValue Parameter;
			Parameter.ExpressionGUID = FGuid(Texture->GetStringField("ExpressionGUID"));

			const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
			if (Texture->TryGetObjectField("ParameterValue", TexturePtr) && TexturePtr != nullptr) {
				LoadObject(TexturePtr, Parameter.ParameterValue);
			}

			TSharedPtr<FJsonObject> ParameterInfoJson = Texture->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;

			TextureParameterValues.Add(Parameter);
		}

		MaterialInstanceConstant->TextureParameterValues = TextureParameterValues;

		TArray<TSharedPtr<FJsonValue>> Local_StaticParameterObjects;
		TArray<TSharedPtr<FJsonValue>> Local_StaticComponentMaskParametersObjects;
		const TSharedPtr<FJsonObject>* StaticParams;

		if (EditorOnlyData.Num() > 0) {
			for (TSharedPtr<FJsonObject> Ed : EditorOnlyData) {
				const TSharedPtr<FJsonObject> Props = Ed->GetObjectField("Properties");

				if (Props->TryGetObjectField("StaticParameters", StaticParams)) {
					for (TSharedPtr<FJsonValue> Parameter : StaticParams->Get()->GetArrayField("StaticSwitchParameters")) {
						Local_StaticParameterObjects.Add(TSharedPtr(Parameter));
					}

					for (TSharedPtr<FJsonValue> Parameter : StaticParams->Get()->GetArrayField("StaticComponentMaskParameters")) {
						Local_StaticComponentMaskParametersObjects.Add(TSharedPtr(Parameter));
					}
				}
			}
		} else if (Properties->TryGetObjectField("StaticParameters", StaticParams)) {
			Local_StaticParameterObjects = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
		} else if (Properties->TryGetObjectField("StaticParametersRuntime", StaticParams)) {
			Local_StaticParameterObjects = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
		}

		TArray<FStaticSwitchParameter> StaticSwitchParameters;
		for (const TSharedPtr<FJsonValue> StaticParameter_Value : Local_StaticParameterObjects) {
			TSharedPtr<FJsonObject> ParameterObject = StaticParameter_Value->AsObject();
			TSharedPtr<FJsonObject> Local_MaterialParameterInfo = ParameterObject->GetObjectField("ParameterInfo");

			// Create Material Parameter Info
			FMaterialParameterInfo MaterialParameterParameterInfo = FMaterialParameterInfo (
				FName(Local_MaterialParameterInfo->GetStringField("Name")),
				static_cast<EMaterialParameterAssociation>(StaticEnum<EMaterialParameterAssociation>()->GetValueByNameString(Local_MaterialParameterInfo->GetStringField("Association"))),
				Local_MaterialParameterInfo->GetIntegerField("Index")
			);
			
			// Now, create the actual switch parameter
			FStaticSwitchParameter Parameter = FStaticSwitchParameter(
				MaterialParameterParameterInfo, 
				ParameterObject->GetBoolField("Value"),
				ParameterObject->GetBoolField("bOverride"),
				FGuid(ParameterObject->GetStringField("ExpressionGUID"))
			);

			StaticSwitchParameters.Add(Parameter);
			MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters.StaticSwitchParameters.Add(Parameter);
		}

		TArray<FStaticComponentMaskParameter> StaticSwitchMaskParameters;
		for (const TSharedPtr<FJsonValue> StaticParameter_Value : Local_StaticComponentMaskParametersObjects) {
			TSharedPtr<FJsonObject> ParameterObject = StaticParameter_Value->AsObject();
			TSharedPtr<FJsonObject> Local_MaterialParameterInfo = ParameterObject->GetObjectField("ParameterInfo");

			// Create Material Parameter Info
			FMaterialParameterInfo MaterialParameterParameterInfo = FMaterialParameterInfo (
				FName(Local_MaterialParameterInfo->GetStringField("Name")),
				static_cast<EMaterialParameterAssociation>(StaticEnum<EMaterialParameterAssociation>()->GetValueByNameString(Local_MaterialParameterInfo->GetStringField("Association"))),
				Local_MaterialParameterInfo->GetIntegerField("Index")
			);

			FStaticComponentMaskParameter Parameter = FStaticComponentMaskParameter(
				MaterialParameterParameterInfo,
				ParameterObject->GetBoolField("R"),
				ParameterObject->GetBoolField("G"),
				ParameterObject->GetBoolField("B"),
				ParameterObject->GetBoolField("A"),
				ParameterObject->GetBoolField("bOverride"),
				FGuid(ParameterObject->GetStringField("ExpressionGUID"))
			);

			StaticSwitchMaskParameters.Add(Parameter);
			MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters.StaticComponentMaskParameters.Add(Parameter);
		}
		
		// Just-in case the material doesn't have the static parameters
		TArray<FName> StaticParameters;
		bool bCheckParameters = false;

		if (MaterialInstanceConstant->GetMaterial() != nullptr && bCheckParameters) {
			UMaterial* Parent = MaterialInstanceConstant->GetMaterial();
			TArray<TObjectPtr<UMaterialExpression>> Expressions = Parent->GetEditorOnlyData()->ExpressionCollection.Expressions;
			FStaticParameterSetEditorOnlyData StaticParams_EditorOnly = MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters;

			// Add parameter to array
			for (UMaterialExpression* Expression : Expressions) {
				if (UMaterialExpressionStaticSwitchParameter* StaticSwitchParameter = Cast<UMaterialExpressionStaticSwitchParameter>(Expression))
					StaticParameters.Add(StaticSwitchParameter->ParameterName);
				if (UMaterialExpressionStaticBoolParameter* StaticBoolParameter = Cast<UMaterialExpressionStaticBoolParameter>(Expression))
					StaticParameters.Add(StaticBoolParameter->ParameterName);
				if (UMaterialExpressionStaticComponentMaskParameter* StaticMaskParameter = Cast<UMaterialExpressionStaticComponentMaskParameter>(Expression))
					StaticParameters.Add(StaticMaskParameter->ParameterName);
			}

			// If a parameter doesn't exist, create it
			for (FStaticSwitchParameter& StaticSwitchParameter : StaticParams_EditorOnly.StaticSwitchParameters) {
				if (!StaticParameters.Contains(StaticSwitchParameter.ParameterInfo.Name)) {
					UMaterialExpressionStaticSwitchParameter* Parameter = NewObject<UMaterialExpressionStaticSwitchParameter>(Parent); {
						Parent->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Parameter);
					}
					Parameter->SetParameterName(StaticSwitchParameter.ParameterInfo.Name);
				}
			}

			// If a parameter doesn't exist, create it
			for (FStaticComponentMaskParameter& StaticSwitchMaskParameter : StaticParams_EditorOnly.StaticComponentMaskParameters) {
				if (!StaticParameters.Contains(StaticSwitchMaskParameter.ParameterInfo.Name)) {
					UMaterialExpressionStaticComponentMaskParameter* Parameter = NewObject<UMaterialExpressionStaticComponentMaskParameter>(Parent); {
						Parent->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Parameter);
					}
					Parameter->SetParameterName(StaticSwitchMaskParameter.ParameterInfo.Name);
				}
			}
		}

		#if IS_PINNACLE
			MaterialInstanceConstant->StaticParametersRuntime.StaticSwitchParameters = StaticSwitchParameters;
		#endif

		// SavePackageHelper(Package, *Package->GetName());
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
