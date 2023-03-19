// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dom/JsonObject.h"

/*
* Global handler for converting JSON to assets
*/
class IImporter {
public:
	IImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {}) {
		this->FileName = FileName;
		this->JsonObject = JsonObject;
		this->Package = Package;
		this->OutermostPkg = OutermostPkg;
		this->AllJsonObjects = AllJsonObjects;
	}

	virtual ~IImporter() {
	}

	/** Import the data of the supported type, return if successful or not */
	virtual bool ImportData() { return false; }

private:
	inline static TArray<FString> AcceptedTypes = {
		"CurveFloat",
		// "CurveVector", // TODO
		"CurveLinearColor",

		"SkeletalMeshLODSettings",
		"Skeleton",

		"AnimSequence",
		"AnimMontage",

		"MaterialFunction",
		"MaterialInstanceConstant",
		"Material",

		"DataTable",
		"ReverbEffect",
		"SoundAttenuation",
		"SubsurfaceProfile",
		"ParticleModuleTypeDataBeam2"
	};

public:
	static bool CanImport(const FString& ImporterType) { return AcceptedTypes.Contains(ImporterType); }

	static bool CanImportAny(TArray<FString>& Types) {
		for (FString& Type : Types) {
			if (!AcceptedTypes.Contains(Type)) continue;
			return true;
		}

		return false;
	}

protected:
	template <typename T>
	T* LoadObject(const TSharedPtr<FJsonObject>* PackageIndex);
	bool HandleAssetCreation(UObject* Asset);

	FName GetExportNameOfSubobject(const FString& PackageIndex);
	TArray<TSharedPtr<FJsonValue>> FilterExportsByOuter(const FString& Outer);

	FString FileName;
	TSharedPtr<FJsonObject> JsonObject;
	UPackage* Package;
	UPackage* OutermostPkg;

	TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
};
