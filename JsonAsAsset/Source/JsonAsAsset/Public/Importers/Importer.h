// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Dom/JsonObject.h"

class IImporter
{
public:
	IImporter(const FString& FileName, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg,
	          const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {})
	{
		this->FileName = FileName;
		this->JsonObject = JsonObject;
		this->Package = Package;
		this->OutermostPkg = OutermostPkg;
		this->AllJsonObjects = AllJsonObjects;
	}

	virtual ~IImporter()
	{
	}

	/** Import the data of the supported type, return if successful or not */
	virtual bool ImportData() { return false; }

private:
	inline static TArray<FString> AcceptedTypes = {
		"CurveFloat",
		"CurveLinearColor",
		"SkeletalMeshLODSettings"
	};

public:
	static bool CanImport(const FString& ImporterType) { return AcceptedTypes.Contains(ImporterType); }

	static bool CanImportAny(const TArray<FString>& Types)
	{
		for (FString Type : Types)
		{
			if (!AcceptedTypes.Contains(Type)) continue;
			return true;
		}

		return false;
	}

protected:
	template <typename T>
	T LoadObject(const TSharedPtr<FJsonObject>* PackageIndex)
	{
		FString Type;
		FString Name;
		PackageIndex->Get()->GetStringField("ObjectName").Split(" ", &Type, &Name);
		FString Path;
		PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
		return Cast<T>(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").TryLoad());
	}

	FString FileName;
	TSharedPtr<FJsonObject> JsonObject;
	UPackage* Package;
	UPackage* OutermostPkg;

	TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
};
