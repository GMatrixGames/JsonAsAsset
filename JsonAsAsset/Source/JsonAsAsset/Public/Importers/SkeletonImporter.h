// Copyright JAA Contributors 2023-2024

#pragma once

#include "Importer.h"

// We use this to set variables
// in the skeletal asset
class USkeletonAssetDerived : public USkeleton {
public:
	bool AddVirtualBone(const FName SourceBoneName, const FName TargetBoneName, const FName VirtualBoneRootName);
};

class USkeletonImporter : public IImporter {
public:
	USkeletonImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, TArray<TSharedPtr<FJsonValue>>& JsonObjects):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, JsonObjects) {
	}

	virtual bool ImportData() override;
};
