// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

class UMaterialFunctionImporter : public IImporter {
public:
	UMaterialFunctionImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;

protected:
	inline static TArray<FString> AcceptedTypes = {
		"MaterialExpressionAbs",
		"MaterialExpressionAdd",
		"MaterialExpressionStaticSwitchParameter",
		"MaterialExpressionFunctionOutput",
		"MaterialExpressionFunctionInput",
		"MaterialExpressionComponentMask",
		"MaterialExpressionConstant",
		"MaterialExpressionConstant2Vector",
		"MaterialExpressionConstant3Vector",
		"MaterialExpressionConstantBiasScale",
		"MaterialExpressionFrac",
		"MaterialExpressionLinearInterpolate",
		"MaterialExpressionOneMinus",
		"MaterialExpressionMultiply",
		"MaterialExpressionVectorParameter",
		"MaterialExpressionTextureSample",
		"MaterialExpressionMaterialFunctionCall",
		"MaterialExpressionTextureObject",
		"MaterialExpressionMax",
		"MaterialExpressionTextureCoordinate",
		"MaterialExpressionTime",
		"MaterialExpressionScalarParameter",
		"MaterialExpressionPanner",
		"MaterialExpressionNamedRerouteDeclaration",
		"MaterialExpressionReroute",
		"MaterialExpressionSubtract",
		"MaterialExpressionSaturate",
		"MaterialExpressionRotator",
		"MaterialExpressionMin",
		"MaterialExpressionNamedRerouteUsage",
		"MaterialExpressionSine",
		"MaterialExpressionSmoothStep",
		"MaterialExpressionAppendVector",
		"MaterialExpressionDivide",
		"MaterialExpressionDistance",
		"MaterialExpressionStaticBool",
		"MaterialExpressionStep",
		"MaterialExpressionDotProduct",
		"MaterialExpressionStaticSwitch",
		"MaterialExpressionPower",
		"MaterialExpressionRound",
		"MaterialExpressionTextureSampleParameter2D",
		"MaterialExpressionFloor",
		"MaterialExpressionCustom",
		"MaterialExpressionCeil",
		"MaterialExpressionIf",
		"MaterialExpressionCosine",
		"MaterialExpressionDesaturation",
		"MaterialExpressionScreenPosition",
		"MaterialExpressionViewSize",
		"MaterialExpressionActorPositionWS",
		"MaterialExpressionTransformPosition",
		"MaterialExpressionObjectRadius",
		"MaterialExpressionPreSkinnedLocalBounds",
		"MaterialExpressionClamp",
		"MaterialExpressionSphereMask",
		"MaterialExpressionCurveAtlasRowParameter",
		"MaterialExpressionObjectPositionWS",
		"MaterialExpressionTextureObjectParameter",
		"MaterialExpressionFmod",
		"MaterialExpressionTextureProperty",
		"MaterialExpressionParticleColor",
		"MaterialExpressionParticlePositionWS",
		"MaterialExpressionWorldPosition",
		"MaterialExpressionNormalize",
		"MaterialExpressionParticleRadius",
		"MaterialExpressionDynamicParameter",
		"MaterialExpressionEyeAdaptation",
		"MaterialExpressionEyeAdaptationInverse",
		"MaterialExpressionCameraPositionWS",
		"MaterialExpressionFeatureLevelSwitch"
	};

	inline static TArray<FString> IgnoredTypes = {
		"MaterialExpressionComposite",
		"MaterialExpressionPinBase"
	};
	
	struct FImportData {
		FImportData(const FName Type, const TSharedPtr<FJsonObject>& Json) {
			this->Type = Type;
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, const TSharedPtr<FJsonObject>& Json) {
			this->Type = FName(Type);
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, FJsonObject* Json) {
			this->Type = FName(Type);
			this->Json = Json;
		}

		FName Type;
		FJsonObject* Json;
	};

	TSharedPtr<FJsonObject> FindEditorOnlyData(const FString& Type, TMap<FName, FImportData>& OutExports);

	TMap<FName, UMaterialExpression*> CreateExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports);

	void AddExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap);

	void AddComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FImportData>& Exports);

	bool AddGenerics(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json);

	UMaterialExpression* CreateEmptyExpression(UObject* Parent, FName Name, FName Type) const;

	FExpressionInput PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type = "Default");
	FExpressionOutput PopulateExpressionOutput(const FJsonObject* JsonProperties);

	FName GetExpressionName(const FJsonObject* JsonProperties);

	FFunctionExpressionOutput PopulateFuncExpressionOutput(const TSharedPtr<FJsonObject>& JsonProperties);
	FFunctionExpressionInput PopulateFuncExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap);
};
