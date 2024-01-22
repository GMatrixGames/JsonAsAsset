// Copyright JAA Contributors 2023-2024

#include "Importers/SkeletalMeshLODSettingsImporter.h"

#include "JsonGlobals.h"
#include "Dom/JsonObject.h"
#include "Factories/DataAssetFactory.h"

void USkeletalMeshLODSettingsDerived::SetLODGroups(TArray<FSkeletalMeshLODGroupSettings> LODGroupsInput) {
	this->LODGroups = LODGroupsInput;
}

void USkeletalMeshLODSettingsDerived::AddLODGroup(FSkeletalMeshLODGroupSettings LODGroupInput) {
	this->LODGroups.Add(LODGroupInput);
}

void USkeletalMeshLODSettingsDerived::EmptyLODGroups() {
	this->LODGroups.Empty();
}

bool USkeletalMeshLODSettingsImporter::ImportData() {
	try {
		UDataAssetFactory* DataAssetFactory = NewObject<UDataAssetFactory>();
		USkeletalMeshLODSettingsDerived* LODDataAsset = Cast<USkeletalMeshLODSettingsDerived>(
			DataAssetFactory->FactoryCreateNew(USkeletalMeshLODSettings::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		// Empty LOD Groups
		LODDataAsset->EmptyLODGroups();

		TArray<TSharedPtr<FJsonValue>> LODGroupsObject = JsonObject->GetObjectField("Properties")->GetArrayField("LODGroups");
		for (int i = 0; i < LODGroupsObject.Num(); i++) {
			TSharedPtr<FJsonObject> LODDataObject = LODGroupsObject[i]->AsObject();
			FSkeletalMeshLODGroupSettings SkeletalMeshLODGroup = FSkeletalMeshLODGroupSettings();

			if (const TSharedPtr<FJsonObject>* BakePose; LODDataObject->TryGetObjectField("BakePose", BakePose) == true) {
				LoadObject(BakePose, SkeletalMeshLODGroup.BakePose);
			}

			SkeletalMeshLODGroup.LODHysteresis = LODDataObject->GetNumberField("LODHysteresis");
			SkeletalMeshLODGroup.WeightOfPrioritization = LODDataObject->GetNumberField("WeightOfPrioritization");
			SkeletalMeshLODGroup.ScreenSize = FPerPlatformFloat(LODDataObject->GetObjectField("ScreenSize")->GetNumberField("Default"));

			if (i != 0) {
				EBoneFilterActionOption BoneFilterActionOption;

				FString StringBoneFilterActionOption = LODDataObject->GetStringField("BoneFilterActionOption");

				if (StringBoneFilterActionOption.EndsWith("Remove")) BoneFilterActionOption = EBoneFilterActionOption::Remove;
				else if (StringBoneFilterActionOption.EndsWith("Keep")) BoneFilterActionOption = EBoneFilterActionOption::Keep;
				else BoneFilterActionOption = EBoneFilterActionOption::Invalid;

				SkeletalMeshLODGroup.BoneFilterActionOption = BoneFilterActionOption;

				for (const TSharedPtr<FJsonValue> BoneListed : LODDataObject->GetArrayField("BoneList")) {
					FBoneFilter BoneFilter;

					BoneFilter.bExcludeSelf = BoneListed->AsObject()->GetBoolField("bExcludeSelf");
					BoneFilter.BoneName = FName(*BoneListed->AsObject()->GetStringField("BoneName"));

					SkeletalMeshLODGroup.BoneList.Add(BoneFilter);
				}
			}

			for (const TSharedPtr<FJsonValue> BonePrioritized : LODDataObject->GetArrayField("BonesToPrioritize"))
				SkeletalMeshLODGroup.BonesToPrioritize.Add(FName(*BonePrioritized->AsString()));

			FSkeletalMeshOptimizationSettings OptimizationSettings;
			TSharedPtr<FJsonObject> ReductionSettings = LODDataObject->GetObjectField("ReductionSettings");

			OptimizationSettings.BaseLOD = ReductionSettings->GetIntegerField("BaseLOD");
			OptimizationSettings.bEnforceBoneBoundaries = ReductionSettings->GetBoolField("bEnforceBoneBoundaries");
			OptimizationSettings.bLockColorBounaries = ReductionSettings->GetBoolField("bLockColorBounaries");
			OptimizationSettings.bLockEdges = ReductionSettings->GetBoolField("bLockEdges");
			OptimizationSettings.bRecalcNormals = ReductionSettings->GetBoolField("bRecalcNormals");
			OptimizationSettings.bRemapMorphTargets = ReductionSettings->GetBoolField("bRemapMorphTargets");
			OptimizationSettings.MaxBonesPerVertex = ReductionSettings->GetIntegerField("MaxBonesPerVertex");
			OptimizationSettings.MaxDeviationPercentage = ReductionSettings->GetNumberField("MaxDeviationPercentage");
			OptimizationSettings.MaxNumOfTriangles = ReductionSettings->GetIntegerField("MaxNumOfTriangles");
			OptimizationSettings.MaxNumOfVerts = ReductionSettings->GetIntegerField("MaxNumOfVerts");
			OptimizationSettings.NormalsThreshold = ReductionSettings->GetNumberField("NormalsThreshold");
			OptimizationSettings.NumOfTrianglesPercentage = ReductionSettings->GetNumberField("NumOfTrianglesPercentage");
			OptimizationSettings.NumOfVertPercentage = ReductionSettings->GetNumberField("NumOfVertPercentage");
			OptimizationSettings.VolumeImportance = ReductionSettings->GetNumberField("VolumeImportance");
			OptimizationSettings.WeldingThreshold = ReductionSettings->GetNumberField("WeldingThreshold");

			SkeletalMeshOptimizationType ReductionMethod;
			FString StringReductionMethod = ReductionSettings->GetStringField("ReductionMethod");

			if (StringReductionMethod.EndsWith("SMOT_NumOfTriangles")) ReductionMethod = SMOT_NumOfTriangles;
			else if (StringReductionMethod.EndsWith("SMOT_MaxDeviation")) ReductionMethod = SMOT_MaxDeviation;
			else if (StringReductionMethod.EndsWith("SMOT_TriangleOrDeviation")) ReductionMethod = SMOT_TriangleOrDeviation;
			else ReductionMethod = SMOT_MAX;

			OptimizationSettings.ReductionMethod = ReductionMethod;

			SkeletalMeshOptimizationImportance ShadingImportance;
			FString StringShadingImportance = ReductionSettings->GetStringField("ShadingImportance");

			if (StringShadingImportance.EndsWith("SMOI_Off")) ShadingImportance = SMOI_Off;
			else if (StringShadingImportance.EndsWith("SMOI_Lowest")) ShadingImportance = SMOI_Lowest;
			else if (StringShadingImportance.EndsWith("SMOI_Low")) ShadingImportance = SMOI_Low;
			else if (StringShadingImportance.EndsWith("SMOI_Normal")) ShadingImportance = SMOI_Normal;
			else if (StringShadingImportance.EndsWith("SMOI_High")) ShadingImportance = SMOI_High;
			else if (StringShadingImportance.EndsWith("SMOI_Highest")) ShadingImportance = SMOI_Highest;
			else ShadingImportance = SMOI_MAX;

			OptimizationSettings.ShadingImportance = ShadingImportance;

			SkeletalMeshOptimizationImportance SilhouetteImportance;
			FString StringSilhouetteImportance = ReductionSettings->GetStringField("SilhouetteImportance");

			if (StringSilhouetteImportance.EndsWith("SMOI_Off")) SilhouetteImportance = SMOI_Off;
			else if (StringSilhouetteImportance.EndsWith("SMOI_Lowest")) SilhouetteImportance = SMOI_Lowest;
			else if (StringSilhouetteImportance.EndsWith("SMOI_Low")) SilhouetteImportance = SMOI_Low;
			else if (StringSilhouetteImportance.EndsWith("SMOI_Normal")) SilhouetteImportance = SMOI_Normal;
			else if (StringSilhouetteImportance.EndsWith("SMOI_High")) SilhouetteImportance = SMOI_High;
			else if (StringSilhouetteImportance.EndsWith("SMOI_Highest")) SilhouetteImportance = SMOI_Highest;
			else SilhouetteImportance = SMOI_MAX;

			OptimizationSettings.SilhouetteImportance = SilhouetteImportance;

			SkeletalMeshOptimizationImportance SkinningImportance;
			FString StringSkinningImportance = ReductionSettings->GetStringField("SkinningImportance");

			if (StringSkinningImportance.EndsWith("SMOI_Off")) SkinningImportance = SMOI_Off;
			else if (StringSkinningImportance.EndsWith("SMOI_Lowest")) SkinningImportance = SMOI_Lowest;
			else if (StringSkinningImportance.EndsWith("SMOI_Low")) SkinningImportance = SMOI_Low;
			else if (StringSkinningImportance.EndsWith("SMOI_Normal")) SkinningImportance = SMOI_Normal;
			else if (StringSkinningImportance.EndsWith("SMOI_High")) SkinningImportance = SMOI_High;
			else if (StringSkinningImportance.EndsWith("SMOI_Highest")) SkinningImportance = SMOI_Highest;
			else SkinningImportance = SMOI_MAX;

			OptimizationSettings.SkinningImportance = SkinningImportance;

			SkeletalMeshTerminationCriterion TerminationCriterion;
			FString StringTerminationCriterion = ReductionSettings->GetStringField("TerminationCriterion");

			if (StringTerminationCriterion.EndsWith("SMOI_Off")) TerminationCriterion = SMTC_NumOfTriangles;
			else if (StringTerminationCriterion.EndsWith("SMOI_Lowest")) TerminationCriterion = SMTC_NumOfVerts;
			else if (StringTerminationCriterion.EndsWith("SMOI_Low")) TerminationCriterion = SMTC_TriangleOrVert;
			else if (StringTerminationCriterion.EndsWith("SMOI_Normal")) TerminationCriterion = SMTC_AbsNumOfTriangles;
			else if (StringTerminationCriterion.EndsWith("SMOI_High")) TerminationCriterion = SMTC_AbsNumOfVerts;
			else if (StringTerminationCriterion.EndsWith("SMOI_Highest")) TerminationCriterion = SMTC_AbsTriangleOrVert;
			else TerminationCriterion = SMTC_MAX;

			OptimizationSettings.TerminationCriterion = TerminationCriterion;

			SkeletalMeshOptimizationImportance TextureImportance;
			FString StringTextureImportance = ReductionSettings->GetStringField("TextureImportance");

			if (StringTextureImportance.EndsWith("SMOI_Off")) TextureImportance = SMOI_Off;
			else if (StringTextureImportance.EndsWith("SMOI_Lowest")) TextureImportance = SMOI_Lowest;
			else if (StringTextureImportance.EndsWith("SMOI_Low")) TextureImportance = SMOI_Low;
			else if (StringTextureImportance.EndsWith("SMOI_Normal")) TextureImportance = SMOI_Normal;
			else if (StringTextureImportance.EndsWith("SMOI_High")) TextureImportance = SMOI_High;
			else if (StringTextureImportance.EndsWith("SMOI_Highest")) TextureImportance = SMOI_Highest;
			else TextureImportance = SMOI_MAX;

			OptimizationSettings.TextureImportance = TextureImportance;
			SkeletalMeshLODGroup.ReductionSettings = OptimizationSettings;
			LODDataAsset->AddLODGroup(SkeletalMeshLODGroup);
		}

		// Handle edit changes, and add it to the content browser
		SavePackage();
		if (!HandleAssetCreation(LODDataAsset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
