// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

// We use this to set variables
// in the LOD asset
class USkeletalMeshLODSettingsDerived : public USkeletalMeshLODSettings {
public:
	void SetLODGroups(TArray<FSkeletalMeshLODGroupSettings> LODGroupsInput);
	void AddLODGroup(FSkeletalMeshLODGroupSettings LODGroupInput);
	void EmptyLODGroups();
};

class USkeletalMeshLODSettingsImporter : public IImporter {
public:
	USkeletalMeshLODSettingsImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
	}

	virtual bool ImportData() override;
};
