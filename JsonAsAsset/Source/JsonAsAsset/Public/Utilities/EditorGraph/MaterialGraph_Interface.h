// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importers/Importer.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

//		Material Graph Handler
// Handles everything needed to create
//   a material graph. (ex: nodes)
class UMaterialGraph_Interface : public IImporter {
public:
	UMaterialGraph_Interface(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	static TArray<FString> GetAcceptedTypes() {
		return Expressions;
	}
protected:
	// Each expression that the plugin has
	// already setup prerequisites for
	inline static TArray<FString> Expressions = {
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
		"MaterialExpressionVirtualTextureFeatureSwitch",
		"MaterialExpressionDistanceFieldsRenderingSwitch",
		"MaterialExpressionRound",
		"MaterialExpressionTextureSampleParameter2D",
		"MaterialExpressionTextureSampleParameter2DArray",
		"MaterialExpressionTextureSampleParameterCube",
		"MaterialExpressionTextureSampleParameterCubeArray",
		"MaterialExpressionTextureSampleParameterSubUV",
		"MaterialExpressionTextureSampleParameterVolume",
		"MaterialExpressionPreviousFrameSwitch",
		"MaterialExpressionShadowReplace",
		"MaterialExpressionLandscapeVisibilityMask",
		"MaterialExpressionVectorNoise",
		"MaterialExpressionFloor",
		"MaterialExpressionCustom",
		"MaterialExpressionCeil",
		"MaterialExpressionIf",
		"MaterialExpressionCosine",
		"MaterialExpressionDesaturation",
		"MaterialExpressionScreenPosition",
		"MaterialExpressionViewSize",
		"MaterialExpressionActorPositionWS",
		"MaterialExpressionRuntimeVirtualTextureSample",
		"MaterialExpressionRuntimeVirtualTextureSampleParameter",
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
		"MaterialExpressionVertexTangentWS",
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
		"MaterialExpressionMaterialAttributeLayers",
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
		"MaterialExpressionLandscapeLayerWeight",
		"MaterialExpressionLandscapePhysicalMaterialOutput",
	};

	inline static TArray<FString> IgnoredExpressions = {
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

	// Find's Material Only Data
	TSharedPtr<FJsonObject> FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FImportData>& OutExports, TArray<FName>& ExpressionNames, bool bFilterByOuter = true);

	//		Functions to Handle Expressions ------------
	void MaterialGraphNode_ExpressionWrapper(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json);
	void MaterialGraphNode_ConstructComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FImportData>& Exports);

	// Makes each expression with their class
	TMap<FName, UMaterialExpression*> ConstructExpressions(UObject* Parent, const FString& Outer, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports);
	UMaterialExpression* CreateEmptyExpression(UObject* Parent, FName Name, FName Type) const;

	// Modifies Graph Nodes (copies over properties from FJsonObject)
	void PropagateExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, bool bCheckOuter = false, bool bSubgraph = false);
	// ----------------------------------------------------

	//		Functions to Handle Node Connections ------------
	FExpressionInput PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type = "Default");
	FExpressionOutput PopulateExpressionOutput(const FJsonObject* JsonProperties);

	FFunctionExpressionOutput PopulateFuncExpressionOutput(const TSharedPtr<FJsonObject>& JsonProperties);
	FFunctionExpressionInput PopulateFuncExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap);

	FName GetExpressionName(const FJsonObject* JsonProperties, FString OverrideParameterName = "Expression");

	FExpressionInput CreateExpressionInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName);
};
