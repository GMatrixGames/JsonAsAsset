// Copyright Epic Games, Inc. All Rights Reserved.

#include "DerivedAssets/SkeletalMeshLODSettingsDerived.h"

void USkeletalMeshLODSettingsDerived::SetLODGroups(TArray<FSkeletalMeshLODGroupSettings> LODGroupsInput) {
	this->LODGroups = LODGroupsInput;
}

void USkeletalMeshLODSettingsDerived::AddLODGroup(FSkeletalMeshLODGroupSettings LODGroupInput) {
	this->LODGroups.Add(LODGroupInput);
}

void USkeletalMeshLODSettingsDerived::EmptyLODGroups() {
	this->LODGroups.Empty();
}