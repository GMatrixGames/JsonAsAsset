// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialInstanceConstantImporter.h"

#include "PackageHelperFunctions.h"
#include "RemoteAssetDownloader.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Utilities/MathUtilities.h"

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
			MaterialInstanceConstant->Parent = LoadObject<UMaterialInterface>(ParentPtr);
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
				FString Type;
				FString Name;
				TexturePtr->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
				FString Path;
				TexturePtr->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
				Name = Name.Replace(TEXT("'"), TEXT(""));

				UTexture2D* Tex = LoadObject<UTexture2D>(TexturePtr);
				if (Tex == nullptr) {
					if (!FRemoteAssetDownloader::MakeTexture(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").ToString(), Tex)) {
						UE_LOG(LogJson, Log, TEXT("Something went wrong here!!"))
					}
				}

				Parameter.ParameterValue = Tex;
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

		TArray<TSharedPtr<FJsonValue>> Static;
		const TSharedPtr<FJsonObject>* StaticParams;

		if (EditorOnlyData.Num() > 0) {
			for (TSharedPtr<FJsonObject> Ed : EditorOnlyData) {
				if (Ed->GetObjectField("Properties")->TryGetObjectField("StaticParameters", StaticParams))
					Static = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
			}
		} else if (Properties->TryGetObjectField("StaticParameters", StaticParams)) {
			Static = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
		} else if (Properties->TryGetObjectField("StaticParametersRuntime", StaticParams)) {
			Static = StaticParams->Get()->GetArrayField("StaticSwitchParameters");
		}

		TArray<FStaticSwitchParameter> StaticSwitchParameters;

		for (int32 i = 0; i < Static.Num(); i++) {
			TSharedPtr<FJsonObject> Stat = Static[i]->AsObject();

			FStaticSwitchParameter Parameter;
			Parameter.ExpressionGUID = FGuid(Stat->GetStringField("ExpressionGUID"));
			Parameter.Value = Stat->GetBoolField("Value");
			Parameter.bOverride = Stat->GetBoolField("bOverride");

			TSharedPtr<FJsonObject> ParameterInfoJson = Stat->GetObjectField("ParameterInfo");
			FMaterialParameterInfo ParameterInfo;
			ParameterInfo.Index = ParameterInfoJson->GetIntegerField("Index");
			ParameterInfo.Name = FName(ParameterInfoJson->GetStringField("Name"));
			ParameterInfo.Association = GlobalParameter;

			Parameter.ParameterInfo = ParameterInfo;

			StaticSwitchParameters.Add(Parameter);
		}

		MaterialInstanceConstant->StaticParametersRuntime.StaticSwitchParameters = StaticSwitchParameters;

		// SavePackageHelper(Package, *Package->GetName());
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
