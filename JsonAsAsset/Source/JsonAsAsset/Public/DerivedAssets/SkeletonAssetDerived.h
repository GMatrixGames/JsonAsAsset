// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"

// We use this to set variables
// in the skeletal asset
class USkeletonAssetDerived : public USkeleton {
public:
	bool AddVirtualBone(const FName SourceBoneName, const FName TargetBoneName, const FName VirtualBoneRootName);
};
