// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

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
		"MaterialExpressionConstant4Vector",
		"MaterialExpressionConstantBiasScale",
		"MaterialExpressionFrac",
		"MaterialExpressionLinearInterpolate",
		"MaterialExpressionInverseLinearInterpolate",
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
		"MaterialExpressionSign",
		"MaterialExpressionSmoothStep",
		"MaterialExpressionAppendVector",
		"MaterialExpressionDivide",
		"MaterialExpressionSkyLightEnvMapSample",
		"MaterialExpressionDistance",
		"MaterialExpressionStaticBool",
		"MaterialExpressionScreenPosition",
		"MaterialExpressionSceneTexelSize",
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
		"MaterialExpressionObjectPositionWS",
		"MaterialExpressionObjectOrientation",
		"MaterialExpressionObjectBounds",
		"MaterialExpressionPreSkinnedLocalBounds",
		"MaterialExpressionClamp",
		"MaterialExpressionSphereMask",
		"MaterialExpressionCurveAtlasRowParameter",
		"MaterialExpressionTextureObjectParameter",
		"MaterialExpressionFmod",
		"MaterialExpressionTextureProperty",
		"MaterialExpressionWorldPosition",
		"MaterialExpressionNormalize",
		"MaterialExpressionArcsine",
		"MaterialExpressionArcsineFast",
		"MaterialExpressionViewSize",
		"MaterialExpressionDynamicParameter",
		"MaterialExpressionEyeAdaptation",
		"MaterialExpressionEyeAdaptationInverse",
		"MaterialExpressionFeatureLevelSwitch",
		"MaterialExpressionCrossProduct",
		"MaterialExpressionDepthFade",
		"MaterialExpressionDeriveNormalZ",
		"MaterialExpressionQualitySwitch",
		"MaterialExpressionReflectionCapturePassSwitch",
		"MaterialExpressionRotateAboutAxis",
		"MaterialExpressionShadingPathSwitch",
		"MaterialExpressionTransform",
		"MaterialExpressionVertexInterpolator",
		"MaterialExpressionVertexNormalWS",
		"UMaterialExpressionVertexTangentWS",
		"MaterialExpressionCameraVectorWS",
		"MaterialExpressionCameraPositionWS",
		"MaterialExpressionCollectionParameter",
		"MaterialExpressionSkyAtmosphereViewLuminance",
		"MaterialExpressionSkyAtmosphereAerialPerspective",
		"MaterialExpressionSkyAtmosphereLightDirection",
		"MaterialExpressionSkyAtmosphereLightIlluminance",
		"MaterialExpressionSkyAtmosphereDistantLightScatteredLuminance",
		"MaterialExpressionSkyAtmosphereLightDiskLuminance",
		"MaterialExpressionBumpOffset",
		"MaterialExpressionFresnel",
		"MaterialExpressionNaniteReplace",
		"MaterialExpressionParticleSubUVProperties",
		"MaterialExpressionParticleSpeed",
		"MaterialExpressionParticleSize",
		"MaterialExpressionParticleRelativeTime",
		"MaterialExpressionParticleRandom",
		"MaterialExpressionParticleRadius",
		"MaterialExpressionParticleColor",
		"MaterialExpressionParticlePositionWS",
		"MaterialExpressionParticleMotionBlurFade",
		"MaterialExpressionParticleMacroUV",
		"MaterialExpressionParticleDirection",
		"MaterialExpressionMaterialProxyReplace",
		"MaterialExpressionSetMaterialAttributes",
		"MaterialExpressionGetMaterialAttributes",
		"MaterialExpressionMakeMaterialAttributes",
		"MaterialExpressionBlendMaterialAttributes",
		"MaterialExpressionBreakMaterialAttributes",
		"MaterialExpressionSquareRoot",
		"MaterialExpressionTwoSidedSign",
		"MaterialExpressionVertexColor",
		"MaterialExpressionTruncate"
		"MaterialExpressionActorPositionWS",
		"MaterialExpressionPixelNormalWS",
		"MaterialExpressionPixelDepth",
		"MaterialExpressionSceneDepth",
		"MaterialExpressionPreSkinnedPosition",
		"MaterialExpressionReflectionVectorWS",
		"MaterialExpressionPrecomputedAOMask",
		"MaterialExpressionShadingModel",
		"MaterialExpressionViewProperty",
		"MaterialExpressionPerInstanceRandom",
		"MaterialExpressionPerInstanceFadeAmount",
		"MaterialExpressionAtmosphericLightVector",
		"MaterialExpressionChannelMaskParameter",
		"MaterialExpressionRayTracingQualitySwitch",
		"MaterialExpressionDistanceCullFade",
		"MaterialExpressionStaticComponentMaskParameter",
		"MaterialExpressionShaderStageSwitch",
		"MaterialExpressionStaticBoolParameter",
		"MaterialExpressionPreSkinnedNormal",
		"MaterialExpressionAbsorptionMediumMaterialOutput",
		"MaterialExpressionNoise",
		"MaterialExpressionEyeAdaptation",
		"MaterialExpressionLightVector",
		"MaterialExpressionLightmapUVs",
		"MaterialExpressionCloudLayer",
		"MaterialExpressionDeltaTime",
		"MaterialExpressionIsOrthographic",
		"MaterialExpressionDDX",
		"MaterialExpressionDDY",
		"MaterialExpressionMaterialLayerOutput",
		"MaterialExpressionArctangent2Fast",
		"MaterialExpressionArctangentFast",
		"MaterialExpressionArctangent2",
		"MaterialExpressionArctangent",
		"MaterialExpressionSceneTexture",
		"MaterialExpressionLandscapeGrassOutput",
		"MaterialExpressionLandscapeLayerSample",
		"MaterialExpressionLandscapeLayerCoords",
		"MaterialExpressionLandscapeLayerSwitch",
		"MaterialExpressionLandscapeLayerBlend",
		"MaterialExpressionLandscapeLayerWeight"
	};

	inline static TArray<FString> IgnoredTypes = {
		"MaterialExpressionComposite",
		"MaterialExpressionPinBase",
		"MaterialExpressionComment",
		"MaterialFunction",
		"Material"
	};

	struct FImportData {
		FImportData(const FName Type, const FName Outer, const TSharedPtr<FJsonObject>& Json) {
			this->Type = Type;
			this->Outer = Outer;
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, const FString& Outer, const TSharedPtr<FJsonObject>& Json) {
			this->Type = FName(Type);
			this->Outer = FName(Outer);
			this->Json = Json.Get();
		}

		FImportData(const FString& Type, const FString& Outer, FJsonObject* Json) {
			this->Type = FName(Type);
			this->Outer = FName(Outer);
			this->Json = Json;
		}

		FName Type;
		FName Outer;
		FJsonObject* Json;
	};

	TSharedPtr<FJsonObject> FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FImportData>& OutExports, TArray<FName>& ExpressionNames, bool bFilterByOuter = true);

	TMap<FName, UMaterialExpression*> CreateExpressions(UObject* Parent, const FString& Outer, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports);

	void AddExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, bool bCheckOuter = false, bool bSubgraph = false);
	void AddComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FImportData>& Exports);
	void AddGenerics(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json);
	void AppendNotification(const FText& Text, SNotificationItem::ECompletionState CompletionState);

	UMaterialExpression* CreateEmptyExpression(UObject* Parent, FName Name, FName Type) const;

	FExpressionInput PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type = "Default");
	FExpressionOutput PopulateExpressionOutput(const FJsonObject* JsonProperties);

	FName GetExpressionName(const FJsonObject* JsonProperties);

	FFunctionExpressionOutput PopulateFuncExpressionOutput(const TSharedPtr<FJsonObject>& JsonProperties);
	FFunctionExpressionInput PopulateFuncExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap);
};
