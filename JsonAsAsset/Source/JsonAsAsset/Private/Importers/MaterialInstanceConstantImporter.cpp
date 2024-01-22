// Copyright JAA Contributors 2023-2024

#include "Importers/MaterialInstanceConstantImporter.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Utilities/MathUtilities.h"
#include "RHIDefinitions.h"

#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
#include "Materials/MaterialExpressionStaticComponentMaskParameter.h"
#include "MaterialShared.h"

bool UMaterialInstanceConstantImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		UMaterialInstanceConstant* MaterialInstanceConstant = NewObject<UMaterialInstanceConstant>(Package, UMaterialInstanceConstant::StaticClass(), *FileName, RF_Public | RF_Standalone);
		HandleAssetCreation(MaterialInstanceConstant);

		TArray<TSharedPtr<FJsonObject>> EditorOnlyData;
		GetObjectSerializer()->DeserializeObjectProperties(Properties, MaterialInstanceConstant);

		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			if (Object->GetStringField("Type") == "MaterialInstanceEditorOnlyData") {
				EditorOnlyData.Add(Object);
			}
		}

		if (const TSharedPtr<FJsonObject>* ParentPtr; Properties->TryGetObjectField("Parent", ParentPtr))
			LoadObject(ParentPtr, MaterialInstanceConstant->Parent);
		if (const TSharedPtr<FJsonObject>* SubsurfaceProfilePtr; Properties->TryGetObjectField("SubsurfaceProfile", SubsurfaceProfilePtr))
			LoadObject(SubsurfaceProfilePtr, MaterialInstanceConstant->SubsurfaceProfile);
		if (bool bOverrideSubsurfaceProfile; Properties->TryGetBoolField("bOverrideSubsurfaceProfile", bOverrideSubsurfaceProfile))
			MaterialInstanceConstant->bOverrideSubsurfaceProfile = bOverrideSubsurfaceProfile;

		TArray<FScalarParameterValue> ScalarParameterValues;
		TArray<TSharedPtr<FJsonValue>> Scalars = Properties->GetArrayField("ScalarParameterValues");

		for (int32 i = 0; i < Scalars.Num(); i++) {
			TSharedPtr<FJsonObject> Scalar = Scalars[i]->AsObject();

			FScalarParameterValue Parameter;
			Parameter.ParameterValue = Scalar->GetNumberField("ParameterValue");
			Parameter.ExpressionGUID = FGuid(Scalar->GetStringField("ExpressionGUID"));
			
			if (const TSharedPtr<FJsonObject>* ParameterInfoPtr; Scalar->TryGetObjectField("ParameterInfo", ParameterInfoPtr)) {
				TSharedPtr<FJsonObject> ParameterInfoJson = Scalar->GetObjectField("ParameterInfo");
				FMaterialParameterInfo ParameterInfo;
				ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
				ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
				ParameterInfo.Association = GlobalParameter;

				Parameter.ParameterInfo = ParameterInfo;
			} else {
				FMaterialParameterInfo ParameterInfo;
				ParameterInfo.Index = -1;
				ParameterInfo.Name = FName(Scalar->GetStringField("ParameterName"));
				ParameterInfo.Association = GlobalParameter;

				Parameter.ParameterInfo = ParameterInfo;
			}

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

			if (const TSharedPtr<FJsonObject>* ParameterInfoPtr; Vector->TryGetObjectField("ParameterInfo", ParameterInfoPtr)) {
				TSharedPtr<FJsonObject> ParameterInfoJson = Vector->GetObjectField("ParameterInfo");
				FMaterialParameterInfo ParameterInfo;
				ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
				ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
				ParameterInfo.Association = GlobalParameter;

				Parameter.ParameterInfo = ParameterInfo;
			} else {
				FMaterialParameterInfo ParameterInfo;
				ParameterInfo.Index = -1;
				ParameterInfo.Name = FName(Vector->GetStringField("ParameterName"));
				ParameterInfo.Association = GlobalParameter;

				Parameter.ParameterInfo = ParameterInfo;
			}

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

			if (const TSharedPtr<FJsonObject>* ParameterInfoPtr; Texture->TryGetObjectField("ParameterInfo", ParameterInfoPtr)) {
				TSharedPtr<FJsonObject> ParameterInfoJson = Texture->GetObjectField("ParameterInfo");
				FMaterialParameterInfo ParameterInfo;
				ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
				ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
				ParameterInfo.Association = GlobalParameter;

				Parameter.ParameterInfo = ParameterInfo;
			} else {
				FMaterialParameterInfo ParameterInfo;
				ParameterInfo.Index = -1;
				ParameterInfo.Name = FName(Texture->GetStringField("ParameterName"));
				ParameterInfo.Association = GlobalParameter;

				Parameter.ParameterInfo = ParameterInfo;
			}

			TextureParameterValues.Add(Parameter);
		}

		MaterialInstanceConstant->TextureParameterValues = TextureParameterValues;

		TArray<TSharedPtr<FJsonValue>> Local_StaticParameterObjects;
		TArray<TSharedPtr<FJsonValue>> Local_StaticComponentMaskParametersObjects;
		const TSharedPtr<FJsonObject>* StaticParams;

		if (Properties->TryGetObjectField("StaticParametersRuntime", StaticParams)) {
			Local_StaticParameterObjects = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
		} else if (EditorOnlyData.Num() > 0) {
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
		}

		// --------- STATIC PARAMETERS -----------
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		FStaticParameterSet NewStaticParameterSet; // Unreal Engine 5.2 and beyond have a different method
#endif

		TArray<FStaticSwitchParameter> StaticSwitchParameters;
		for (const TSharedPtr<FJsonValue> StaticParameter_Value : Local_StaticParameterObjects) {
			TSharedPtr<FJsonObject> ParameterObject = StaticParameter_Value->AsObject();
			TSharedPtr<FJsonObject> Local_MaterialParameterInfo = ParameterObject->GetObjectField("ParameterInfo");

			// Create Material Parameter Info
			FMaterialParameterInfo MaterialParameterParameterInfo = FMaterialParameterInfo(
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
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
				MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters.StaticSwitchParameters.Add(Parameter);
			#endif
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
				// Unreal Engine 5.2 and beyond have a different method
				NewStaticParameterSet.StaticSwitchParameters.Add(Parameter); 
			#endif
		}

		TArray<FStaticComponentMaskParameter> StaticSwitchMaskParameters;
		for (const TSharedPtr<FJsonValue> StaticParameter_Value : Local_StaticComponentMaskParametersObjects) {
			TSharedPtr<FJsonObject> ParameterObject = StaticParameter_Value->AsObject();
			TSharedPtr<FJsonObject> Local_MaterialParameterInfo = ParameterObject->GetObjectField("ParameterInfo");

			// Create Material Parameter Info
			FMaterialParameterInfo MaterialParameterParameterInfo = FMaterialParameterInfo(
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			MaterialInstanceConstant->GetEditorOnlyData()->StaticParameters.StaticComponentMaskParameters.Add(Parameter);
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
			NewStaticParameterSet.EditorOnly.StaticComponentMaskParameters.Add(Parameter);
#endif
		}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		FMaterialUpdateContext MaterialUpdateContext(FMaterialUpdateContext::EOptions::Default & ~FMaterialUpdateContext::EOptions::RecreateRenderStates);
	
		MaterialInstanceConstant->UpdateStaticPermutation(NewStaticParameterSet, &MaterialUpdateContext);
		MaterialInstanceConstant->InitStaticPermutation();
#endif

		SavePackage();
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
