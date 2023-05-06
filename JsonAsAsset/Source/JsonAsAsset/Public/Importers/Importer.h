// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dom/JsonObject.h"
#include "Utilities/ObjectUtilities.h"
#include "Utilities/PropertyUtilities.h"
#include "Widgets/Notifications/SNotificationList.h"

/*
* Global handler for converting JSON to assets
*/
class IImporter {
public:
	IImporter() :
		Package(nullptr),
		OutermostPkg(nullptr) {
	}

	IImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {}) {
		this->FileName = FileName;
		this->FilePath = FilePath;
		this->JsonObject = JsonObject;
		this->Package = Package;
		this->OutermostPkg = OutermostPkg;
		this->AllJsonObjects = AllJsonObjects;
		this->PropertySerializer = NewObject<UPropertySerializer>();
		this->GObjectSerializer = NewObject<UObjectSerializer>();
		this->GObjectSerializer->SetPropertySerializer(PropertySerializer);
	}

	virtual ~IImporter() {
	}

	/** Import the data of the supported type, return if successful or not */
	virtual bool ImportData() { return false; }

private:
	UPROPERTY()
		UObjectSerializer* GObjectSerializer;
	UPROPERTY()
		UPropertySerializer* PropertySerializer;

	inline static TArray<FString> AcceptedTypes = {
		"CurveFloat",
		"CurveVector",
		"CurveLinearColor",
		"CurveLinearColorAtlas",

		// separator

		"SkeletalMeshLODSettings",
		"Skeleton",

		// separator

		"AnimSequence",
		"AnimMontage",

		// separator

		"Material",
		"MaterialFunction",
		"MaterialInstanceConstant",
		"MaterialParameterCollection",
		"NiagaraParameterCollection",

		// separator

		"DataTable",
		"LandscapeGrassType",

		// separator

		"ReverbEffect",
		"SoundAttenuation",
		"SoundConcurrency",

		// separator

		"SubsurfaceProfile",

		// separator

		"TextureRenderTarget2D",
		"PhysicalMaterial",
	};

public:
	template <class T = UObject>
	void LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object);
	template <class T = UObject>
	TArray<TObjectPtr<T>> LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array);

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

	void ImportReference(const FString& File);
	bool HandleReference(const FString& GamePath);

	bool HandleExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, bool bHideNotifications = false);

	virtual void AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons = false, float WidthOverride = 500);
	virtual void AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, const FSlateBrush* SlateBrush, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons = false, float WidthOverride = 500);

protected:
	bool HandleAssetCreation(UObject* Asset) const;

	// Wrapper for remote downloading
	template <class T = UObject>
	TObjectPtr<T> DownloadWrapper(TObjectPtr<T> InObject, FString Type, FString Name, FString Path);

	FName GetExportNameOfSubobject(const FString& PackageIndex);
	TArray<TSharedPtr<FJsonValue>> FilterExportsByOuter(const FString& Outer);
	TSharedPtr<FJsonValue> GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object);

	FORCEINLINE UObjectSerializer* GetObjectSerializer() const { return GObjectSerializer; }
	FString FileName;
	FString FilePath;
	TSharedPtr<FJsonObject> JsonObject;
	UPackage* Package;
	UPackage* OutermostPkg;

	TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
};
