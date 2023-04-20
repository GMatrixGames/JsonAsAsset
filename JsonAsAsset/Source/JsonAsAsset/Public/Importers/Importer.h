// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dom/JsonObject.h"

/*
* Global handler for converting JSON to assets
*/
class IImporter {
public:
	IImporter() {
	}
	IImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {}) {
		this->FileName = FileName;
		this->FilePath = FilePath;
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

		"Material",
		"MaterialFunction",
		"MaterialInstanceConstant",
		"MaterialParameterCollection",

		"DataTable",
		"ReverbEffect",
		"SoundAttenuation",
		"SubsurfaceProfile",

		"TextureRenderTarget2D"
	};

public:
	static bool CanImport(const FString& ImporterType) { return AcceptedTypes.Contains(ImporterType); }

	static bool CanImportAny(TArray<FString>& Types) {
		for (FString& Type : Types) {
			if (!CanImport(Type)) continue;
			return true;
		}

		return false;
	}

	static TArray<FString> GetAcceptedTypes() {
		return AcceptedTypes;
	}

	void ImportReference(FString File);
	bool HandleReference(FString GamePath);
protected:
	template <class T = UObject>
	void LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object);
	bool HandleAssetCreation(UObject* Asset);

	FName GetExportNameOfSubobject(const FString& PackageIndex);
	TArray<TSharedPtr<FJsonValue>> FilterExportsByOuter(const FString& Outer);
	TSharedPtr<FJsonValue> GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object);

	FString FileName;
	FString FilePath;
	TSharedPtr<FJsonObject> JsonObject;
	UPackage* Package;
	UPackage* OutermostPkg;

	TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
};
