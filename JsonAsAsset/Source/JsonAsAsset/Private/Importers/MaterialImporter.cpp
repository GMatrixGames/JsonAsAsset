// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialImporter.h"

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3) || ENGINE_MAJOR_VERSION == 4
#include "Materials/Material.h"
#else
#include "MaterialDomain.h"
#endif
#include "Dom/JsonObject.h"
#include "Factories/MaterialFactoryNew.h"
#include "Utilities/MathUtilities.h"
#include "MaterialEditor/Private/MaterialEditor.h"
#include "MaterialGraph/MaterialGraph.h"
#include <Editor/UnrealEd/Classes/MaterialGraph/MaterialGraphNode_Composite.h>
#include <Editor/UnrealEd/Classes/MaterialGraph/MaterialGraphSchema.h>

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
		Material->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));

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

		bool AllowTranslucentCustomDepthWrites;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("AllowTranslucentCustomDepthWrites", AllowTranslucentCustomDepthWrites)) Material->AllowTranslucentCustomDepthWrites = AllowTranslucentCustomDepthWrites;
		bool bAllowDevelopmentShaderCompile;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bAllowDevelopmentShaderCompile", bAllowDevelopmentShaderCompile)) Material->bAllowDevelopmentShaderCompile = bAllowDevelopmentShaderCompile;
		bool bAllowNegativeEmissiveColor;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bAllowNegativeEmissiveColor", bAllowNegativeEmissiveColor)) Material->bAllowNegativeEmissiveColor = bAllowNegativeEmissiveColor;
		bool bApplyCloudFogging;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bApplyCloudFogging", bApplyCloudFogging)) Material->bApplyCloudFogging = bApplyCloudFogging;
		bool bAutomaticallySetUsageInEditor;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bAutomaticallySetUsageInEditor", bAutomaticallySetUsageInEditor)) Material->bAutomaticallySetUsageInEditor = bAutomaticallySetUsageInEditor;
		bool bCanMaskedBeAssumedOpaque;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bCanMaskedBeAssumedOpaque", bCanMaskedBeAssumedOpaque)) Material->bCanMaskedBeAssumedOpaque = bCanMaskedBeAssumedOpaque;
		bool bCastDynamicShadowAsMasked;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bCastDynamicShadowAsMasked", bCastDynamicShadowAsMasked)) Material->bCastDynamicShadowAsMasked = bCastDynamicShadowAsMasked;
		bool bCastRayTracedShadows;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bCastRayTracedShadows", bCastRayTracedShadows)) Material->bCastRayTracedShadows = bCastRayTracedShadows;
		bool bComputeFogPerPixel;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bComputeFogPerPixel", bComputeFogPerPixel)) Material->bComputeFogPerPixel = bComputeFogPerPixel;
		bool bContactShadows;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bContactShadows", bContactShadows)) Material->bContactShadows = bContactShadows;
		bool bDisableDepthTest;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bDisableDepthTest", bDisableDepthTest)) Material->bDisableDepthTest = bDisableDepthTest;
		bool bEnableMobileSeparateTranslucency;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bEnableMobileSeparateTranslucency", bEnableMobileSeparateTranslucency)) Material->bEnableMobileSeparateTranslucency = bEnableMobileSeparateTranslucency;
		bool bEnableStencilTest;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bEnableStencilTest", bEnableStencilTest)) Material->bEnableStencilTest = bEnableStencilTest;
		bool bForwardBlendsSkyLightCubemaps;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bForwardBlendsSkyLightCubemaps", bForwardBlendsSkyLightCubemaps)) Material->bForwardBlendsSkyLightCubemaps = bForwardBlendsSkyLightCubemaps;
		bool bForwardRenderUsePreintegratedGFForSimpleIBL;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bForwardRenderUsePreintegratedGFForSimpleIBL", bForwardRenderUsePreintegratedGFForSimpleIBL)) Material->bForwardRenderUsePreintegratedGFForSimpleIBL = bForwardRenderUsePreintegratedGFForSimpleIBL;
		bool bFullyRough;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bFullyRough", bFullyRough)) Material->bFullyRough = bFullyRough;
		bool bGenerateSphericalParticleNormals;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bGenerateSphericalParticleNormals", bGenerateSphericalParticleNormals)) Material->bGenerateSphericalParticleNormals = bGenerateSphericalParticleNormals;
		bool bIsBlendable;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bIsBlendable", bIsBlendable)) Material->bIsBlendable = bIsBlendable;
		bool bIsFunctionPreviewMaterial;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bIsFunctionPreviewMaterial", bIsFunctionPreviewMaterial)) Material->bIsFunctionPreviewMaterial = bIsFunctionPreviewMaterial;
		bool bIsMaterialEditorStatsMaterial;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bIsMaterialEditorStatsMaterial", bIsMaterialEditorStatsMaterial)) Material->bIsMaterialEditorStatsMaterial = bIsMaterialEditorStatsMaterial;
		bool bIsPreviewMaterial;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bIsPreviewMaterial", bIsPreviewMaterial)) Material->bIsPreviewMaterial = bIsPreviewMaterial;
		bool bIsSky;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bIsSky", bIsSky)) Material->bIsSky = bIsSky;
		bool BlendableOutputAlpha;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("BlendableOutputAlpha", BlendableOutputAlpha)) Material->BlendableOutputAlpha = BlendableOutputAlpha;
		bool bNormalCurvatureToRoughness;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bNormalCurvatureToRoughness", bNormalCurvatureToRoughness)) Material->bNormalCurvatureToRoughness = bNormalCurvatureToRoughness;
		bool bOutputTranslucentVelocity;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bOutputTranslucentVelocity", bOutputTranslucentVelocity)) Material->bOutputTranslucentVelocity = bOutputTranslucentVelocity;
		bool bScreenSpaceReflections;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bScreenSpaceReflections", bScreenSpaceReflections)) Material->bScreenSpaceReflections = bScreenSpaceReflections;
		bool bTangentSpaceNormal;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bTangentSpaceNormal", bTangentSpaceNormal)) Material->bTangentSpaceNormal = bTangentSpaceNormal;
		bool bUseAlphaToCoverage;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUseAlphaToCoverage", bUseAlphaToCoverage)) Material->bUseAlphaToCoverage = bUseAlphaToCoverage;
		bool bUsedAsSpecialEngineMaterial;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedAsSpecialEngineMaterial", bUsedAsSpecialEngineMaterial)) Material->bUsedAsSpecialEngineMaterial = bUsedAsSpecialEngineMaterial;
		bool bUsedWithBeamTrails;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithBeamTrails", bUsedWithBeamTrails)) Material->bUsedWithBeamTrails = bUsedWithBeamTrails;
		bool bUsedWithClothing;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithClothing", bUsedWithClothing)) Material->bUsedWithClothing = bUsedWithClothing;
		bool bUsedWithEditorCompositing;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithEditorCompositing", bUsedWithEditorCompositing)) Material->bUsedWithEditorCompositing = bUsedWithEditorCompositing;
		bool bUsedWithGeometryCache;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithGeometryCache", bUsedWithGeometryCache)) Material->bUsedWithGeometryCache = bUsedWithGeometryCache;
		bool bUsedWithGeometryCollections;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithGeometryCollections", bUsedWithGeometryCollections)) Material->bUsedWithGeometryCollections = bUsedWithGeometryCollections;
		bool bUsedWithHairStrands;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithHairStrands", bUsedWithHairStrands)) Material->bUsedWithHairStrands = bUsedWithHairStrands;
		bool bUsedWithInstancedStaticMeshes;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithInstancedStaticMeshes", bUsedWithInstancedStaticMeshes)) Material->bUsedWithInstancedStaticMeshes = bUsedWithInstancedStaticMeshes;
		bool bUsedWithLidarPointCloud;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithLidarPointCloud", bUsedWithLidarPointCloud)) Material->bUsedWithLidarPointCloud = bUsedWithLidarPointCloud;
		bool bUsedWithMeshParticles;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithMeshParticles", bUsedWithMeshParticles)) Material->bUsedWithMeshParticles = bUsedWithMeshParticles;
		bool bUsedWithMorphTargets;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithMorphTargets", bUsedWithMorphTargets)) Material->bUsedWithMorphTargets = bUsedWithMorphTargets;
		bool bUsedWithNiagaraMeshParticles;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithNiagaraMeshParticles", bUsedWithNiagaraMeshParticles)) Material->bUsedWithNiagaraMeshParticles = bUsedWithNiagaraMeshParticles;
		bool bUsedWithNiagaraRibbons;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithNiagaraRibbons", bUsedWithNiagaraRibbons)) Material->bUsedWithNiagaraRibbons = bUsedWithNiagaraRibbons;
		bool bUsedWithNiagaraSprites;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithNiagaraSprites", bUsedWithNiagaraSprites)) Material->bUsedWithNiagaraSprites = bUsedWithNiagaraSprites;
		bool bUsedWithParticleSprites;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithParticleSprites", bUsedWithParticleSprites)) Material->bUsedWithParticleSprites = bUsedWithParticleSprites;
		bool bUsedWithSkeletalMesh;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithSkeletalMesh", bUsedWithSkeletalMesh)) Material->bUsedWithSkeletalMesh = bUsedWithSkeletalMesh;
		bool bUsedWithSplineMeshes;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithSplineMeshes", bUsedWithSplineMeshes)) Material->bUsedWithSplineMeshes = bUsedWithSplineMeshes;
		bool bUsedWithStaticLighting;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithStaticLighting", bUsedWithStaticLighting)) Material->bUsedWithStaticLighting = bUsedWithStaticLighting;
		bool bUsedWithVirtualHeightfieldMesh;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithVirtualHeightfieldMesh", bUsedWithVirtualHeightfieldMesh)) Material->bUsedWithVirtualHeightfieldMesh = bUsedWithVirtualHeightfieldMesh;
		bool bUsedWithWater;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsedWithWater", bUsedWithWater)) Material->bUsedWithWater = bUsedWithWater;
		bool bUseEmissiveForDynamicAreaLighting;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUseEmissiveForDynamicAreaLighting", bUseEmissiveForDynamicAreaLighting)) Material->bUseEmissiveForDynamicAreaLighting = bUseEmissiveForDynamicAreaLighting;
		bool bUseHQForwardReflections;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUseHQForwardReflections", bUseHQForwardReflections)) Material->bUseHQForwardReflections = bUseHQForwardReflections;
		bool bUseLightmapDirectionality;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUseLightmapDirectionality", bUseLightmapDirectionality)) Material->bUseLightmapDirectionality = bUseLightmapDirectionality;
		bool bUseMaterialAttributes;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUseMaterialAttributes", bUseMaterialAttributes)) Material->bUseMaterialAttributes = bUseMaterialAttributes;
		bool bUsePlanarForwardReflections;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsePlanarForwardReflections", bUsePlanarForwardReflections)) Material->bUsePlanarForwardReflections = bUsePlanarForwardReflections;
		bool bUsesDistortion;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUsesDistortion", bUsesDistortion)) Material->bUsesDistortion = bUsesDistortion;
		bool bUseTranslucencyVertexFog;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bUseTranslucencyVertexFog", bUseTranslucencyVertexFog)) Material->bUseTranslucencyVertexFog = bUseTranslucencyVertexFog;
		bool bWriteOnlyAlpha;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bWriteOnlyAlpha", bWriteOnlyAlpha)) Material->bWriteOnlyAlpha = bWriteOnlyAlpha;
		bool DitheredLODTransition;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("DitheredLODTransition", DitheredLODTransition)) Material->DitheredLODTransition = DitheredLODTransition;
		bool DitherOpacityMask;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("DitherOpacityMask", DitherOpacityMask)) Material->DitherOpacityMask = DitherOpacityMask;
		bool TwoSided;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("TwoSided", TwoSided)) Material->TwoSided = TwoSided;
		bool Wireframe;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("Wireframe", Wireframe)) Material->Wireframe = Wireframe;

		int32 BlendablePriority;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("BlendablePriority", BlendablePriority)) Material->BlendablePriority = BlendablePriority;
		int32 EditorPitch;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("EditorPitch", EditorPitch)) Material->EditorPitch = EditorPitch;
		int32 EditorX;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("EditorX", EditorX)) Material->EditorX = EditorX;
		int32 EditorY;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("EditorY", EditorY)) Material->EditorY = EditorY;
		int32 EditorYaw;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("EditorYaw", EditorYaw)) Material->EditorYaw = EditorYaw;
		int32 NumCustomizedUVs;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("NumCustomizedUVs", NumCustomizedUVs)) Material->NumCustomizedUVs = NumCustomizedUVs;

		float OpacityMaskClipValue;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("OpacityMaskClipValue", OpacityMaskClipValue)) Material->OpacityMaskClipValue = OpacityMaskClipValue;
		float RefractionDepthBias;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("RefractionDepthBias", RefractionDepthBias)) Material->RefractionDepthBias = RefractionDepthBias;
		float TranslucencyDirectionalLightingIntensity;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucencyDirectionalLightingIntensity", TranslucencyDirectionalLightingIntensity)) Material->TranslucencyDirectionalLightingIntensity = TranslucencyDirectionalLightingIntensity;
		float TranslucentBackscatteringExponent;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucentBackscatteringExponent", TranslucentBackscatteringExponent)) Material->TranslucentBackscatteringExponent = TranslucentBackscatteringExponent;
		float TranslucentSelfShadowDensityScale;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucentSelfShadowDensityScale", TranslucentSelfShadowDensityScale)) Material->TranslucentSelfShadowDensityScale = TranslucentSelfShadowDensityScale;
		float TranslucentSelfShadowSecondDensityScale;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucentSelfShadowSecondDensityScale", TranslucentSelfShadowSecondDensityScale)) Material->TranslucentSelfShadowSecondDensityScale = TranslucentSelfShadowSecondDensityScale;
		float TranslucentSelfShadowSecondOpacity;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucentSelfShadowSecondOpacity", TranslucentSelfShadowSecondOpacity)) Material->TranslucentSelfShadowSecondOpacity = TranslucentSelfShadowSecondOpacity;
		float TranslucentShadowDensityScale;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucentShadowDensityScale", TranslucentShadowDensityScale)) Material->TranslucentShadowDensityScale = TranslucentShadowDensityScale;
		float TranslucentShadowStartOffset;
		if (JsonObject->GetObjectField("Properties")->TryGetNumberField("TranslucentShadowStartOffset", TranslucentShadowStartOffset)) Material->TranslucentShadowStartOffset = TranslucentShadowStartOffset;

		const TSharedPtr<FJsonObject>* TranslucentMultipleScatteringExtinction;
		if (JsonObject->TryGetObjectField("TranslucentMultipleScatteringExtinction", TranslucentMultipleScatteringExtinction)) Material->TranslucentMultipleScatteringExtinction = FMathUtilities::ObjectToLinearColor(TranslucentMultipleScatteringExtinction->Get());

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(Material)) return false;

		// Clear any default expressions the engine adds (ex: Result)
		Material->GetExpressionCollection().Empty();

		// Define editor only data from the JSON
		TMap<FName, FImportData> Exports;
		TArray<FName> ExpressionNames;
		TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), Material->GetName(), Exports, ExpressionNames, false)->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = CreateExpressions(Material, Material->GetName(), ExpressionNames, Exports);

		const TSharedPtr<FJsonObject>* BaseColorPtr;
		if (EdProps->TryGetObjectField("BaseColor", BaseColorPtr) && BaseColorPtr != nullptr) {
			FJsonObject* BaseColorObject = BaseColorPtr->Get();
			FName BaseColorExpressionName = GetExpressionName(BaseColorObject);

			if (CreatedExpressionMap.Contains(BaseColorExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(BaseColorObject, *CreatedExpressionMap.Find(BaseColorExpressionName), "Color");
				FColorMaterialInput* BaseColor = reinterpret_cast<FColorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->BaseColor = *BaseColor;
			}
		}

		const TSharedPtr<FJsonObject>* MetallicPtr;
		if (EdProps->TryGetObjectField("Metallic", MetallicPtr) && MetallicPtr != nullptr) {
			FJsonObject* MetallicObject = MetallicPtr->Get();
			FName MetallicExpressionName = GetExpressionName(MetallicObject);

			if (CreatedExpressionMap.Contains(MetallicExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(MetallicObject, *CreatedExpressionMap.Find(MetallicExpressionName), "Scalar");
				FScalarMaterialInput* Metallic = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Metallic = *Metallic;
			}
		}

		const TSharedPtr<FJsonObject>* SpecularPtr;
		if (EdProps->TryGetObjectField("Specular", SpecularPtr) && SpecularPtr != nullptr) {
			FJsonObject* SpecularObject = SpecularPtr->Get();
			FName SpecularExpressionName = GetExpressionName(SpecularObject);

			if (CreatedExpressionMap.Contains(SpecularExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(SpecularObject, *CreatedExpressionMap.Find(SpecularExpressionName), "Scalar");
				FScalarMaterialInput* Specular = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Specular = *Specular;
			}
		}

		const TSharedPtr<FJsonObject>* RoughnessPtr;
		if (EdProps->TryGetObjectField("Roughness", RoughnessPtr) && RoughnessPtr != nullptr) {
			FJsonObject* RoughnessObject = RoughnessPtr->Get();
			FName RoughnessExpressionName = GetExpressionName(RoughnessObject);

			if (CreatedExpressionMap.Contains(RoughnessExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(RoughnessObject, *CreatedExpressionMap.Find(RoughnessExpressionName), "Scalar");
				FScalarMaterialInput* Roughness = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Roughness = *Roughness;
			}
		}

		const TSharedPtr<FJsonObject>* AnisotropyPtr;
		if (EdProps->TryGetObjectField("Anisotropy", AnisotropyPtr) && AnisotropyPtr != nullptr) {
			FJsonObject* AnisotropyObject = AnisotropyPtr->Get();
			FName AnisotropyExpressionName = GetExpressionName(AnisotropyObject);

			if (CreatedExpressionMap.Contains(AnisotropyExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(AnisotropyObject, *CreatedExpressionMap.Find(AnisotropyExpressionName), "Scalar");
				FScalarMaterialInput* Anisotropy = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Anisotropy = *Anisotropy;
			}
		}

		const TSharedPtr<FJsonObject>* NormalPtr;
		if (EdProps->TryGetObjectField("Normal", NormalPtr) && NormalPtr != nullptr) {
			FJsonObject* NormalObject = NormalPtr->Get();
			FName NormalExpressionName = GetExpressionName(NormalObject);

			if (CreatedExpressionMap.Contains(NormalExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(NormalObject, *CreatedExpressionMap.Find(NormalExpressionName), "Vector");
				FVectorMaterialInput* Normal = reinterpret_cast<FVectorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Normal = *Normal;
			}
		}

		const TSharedPtr<FJsonObject>* TangentPtr;
		if (EdProps->TryGetObjectField("Tangent", TangentPtr) && TangentPtr != nullptr) {
			FJsonObject* TangentObject = TangentPtr->Get();
			FName TangentExpressionName = GetExpressionName(TangentObject);

			if (CreatedExpressionMap.Contains(TangentExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(TangentObject, *CreatedExpressionMap.Find(TangentExpressionName), "Vector");
				FVectorMaterialInput* Tangent = reinterpret_cast<FVectorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->Tangent = *Tangent;
			}
		}

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

		const TSharedPtr<FJsonObject>* SubsurfaceColorPtr;
		if (EdProps->TryGetObjectField("SubsurfaceColor", SubsurfaceColorPtr) && SubsurfaceColorPtr != nullptr) {
			FJsonObject* SubsurfaceColorObject = SubsurfaceColorPtr->Get();
			FName SubsurfaceColorExpressionName = GetExpressionName(SubsurfaceColorObject);

			if (CreatedExpressionMap.Contains(SubsurfaceColorExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(SubsurfaceColorObject, *CreatedExpressionMap.Find(SubsurfaceColorExpressionName), "Color");
				FColorMaterialInput* SubsurfaceColor = reinterpret_cast<FColorMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->SubsurfaceColor = *SubsurfaceColor;
			}
		}

		const TSharedPtr<FJsonObject>* ClearCoatPtr;
		if (EdProps->TryGetObjectField("ClearCoat", ClearCoatPtr) && ClearCoatPtr != nullptr) {
			FJsonObject* ClearCoatObject = ClearCoatPtr->Get();
			FName ClearCoatExpressionName = GetExpressionName(ClearCoatObject);

			if (CreatedExpressionMap.Contains(ClearCoatExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(ClearCoatObject, *CreatedExpressionMap.Find(ClearCoatExpressionName), "Scalar");
				FScalarMaterialInput* ClearCoat = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->ClearCoat = *ClearCoat;
			}
		}

		const TSharedPtr<FJsonObject>* AmbientOcclusionPtr;
		if (EdProps->TryGetObjectField("AmbientOcclusion", AmbientOcclusionPtr) && AmbientOcclusionPtr != nullptr) {
			FJsonObject* AmbientOcclusionObject = AmbientOcclusionPtr->Get();
			FName AmbientOcclusionExpressionName = GetExpressionName(AmbientOcclusionObject);

			if (CreatedExpressionMap.Contains(AmbientOcclusionExpressionName)) {
				FExpressionInput Ex = PopulateExpressionInput(AmbientOcclusionObject, *CreatedExpressionMap.Find(AmbientOcclusionExpressionName), "Scalar");
				FScalarMaterialInput* AmbientOcclusion = reinterpret_cast<FScalarMaterialInput*>(&Ex);
				Material->GetEditorOnlyData()->AmbientOcclusion = *AmbientOcclusion;
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

		// Iterate through all the expression names
		AddExpressions(Material, ExpressionNames, Exports, CreatedExpressionMap, true);
		AddComments(Material, StringExpressionCollection, Exports);

		// Create Editor
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		FMaterialEditor* AssetEditorInstance = reinterpret_cast<FMaterialEditor*>(AssetEditorSubsystem->OpenEditorForAsset(Material) ? AssetEditorSubsystem->FindEditorForAsset(Material, true) : nullptr);

		// Handle Material Graphs
		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			FString ExType = Object->GetStringField("Type");
			FString Name = Object->GetStringField("Name");

			if (ExType == "MaterialGraph" && Name != "MaterialGraph_0") {
				TSharedPtr<FJsonObject> Properties = Object->GetObjectField("Properties");
				TSharedPtr<FJsonObject> SubgraphExpression;

				FString SubgraphExpressionName;

				// Set SubgraphExpression
				const TSharedPtr<FJsonObject>* SubgraphExpressionPtr = nullptr;
				if (Properties->TryGetObjectField("SubgraphExpression", SubgraphExpressionPtr) && SubgraphExpressionPtr != nullptr) {
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

					AddGenerics(Material, CompositeExpression, SubgraphExpression);
				}

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

						UMaterialExpression* Ex = CreateEmptyExpression(MaterialGraph->Material, GraphNodeNameName, FName(GraphNode_Type));
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
					AddExpressions(MaterialGraph->Material, SubGraphExpressionNames, Exports, SubgraphExpressionMapping, true);

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

		AssetEditorInstance->UpdateMaterialAfterGraphChange();
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
