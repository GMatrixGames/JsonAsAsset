// Copyright Epic Games, Inc. All Rights Reserved.

#include "DerivedAssets/SkeletonAssetDerived.h"

bool USkeletonAssetDerived::AddVirtualBone(const FName SourceBoneName, const FName TargetBoneName, const FName VirtualBoneRootName) {
	for (const FVirtualBone& SSBone : VirtualBones) {
		if (SSBone.SourceBoneName == SourceBoneName &&
			SSBone.TargetBoneName == TargetBoneName)
		{
			return false;
		}
	}

	Modify();

	FVirtualBone VirtualBone = FVirtualBone(SourceBoneName, TargetBoneName);
	VirtualBone.VirtualBoneName = VirtualBoneRootName;

	VirtualBones.Add(VirtualBone);

	VirtualBoneGuid = FGuid::NewGuid();
	check(VirtualBoneGuid.IsValid());

	return true;
}