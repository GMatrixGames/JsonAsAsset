// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMeshLODSettings.h"

// We use this to set variables
// in the LOD asset
class USkeletalMeshLODSettingsDerived : public USkeletalMeshLODSettings {
public:
	void SetLODGroups(TArray<FSkeletalMeshLODGroupSettings> LODGroupsInput);
	void AddLODGroup(FSkeletalMeshLODGroupSettings LODGroupInput);
	void EmptyLODGroups();
};
