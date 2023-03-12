// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct ImportOptionsBase
{
	bool bImportWithPath;
};

class FToolBarBuilder;
class FMenuBuilder;

class FJsonAsAssetModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command. */
	void PluginButtonClicked();

private:
	void RegisterMenus();

	TSharedPtr<FUICommandList> PluginCommands;
	FVector ObjectToVector(FJsonObject* Object);
	FRotator ObjectToRotator(FJsonObject* Object);
	FQuat ObjectToQuat(FJsonObject* Object);
	FLinearColor ObjectToLinearColor(FJsonObject* Object);
	
	// Creates a dialog for a file
	TArray<FString> OpenFileDialog(FString Title, FString Type);

	UObject* GetSelectedAsset();
	UPackage* CreateAssetPackage(const FString& Name, const TArray<FString>& Files) const;
	UPackage* CreateAssetPackage(const FString& Name, const TArray<FString>& Files, UPackage*& OutOutermostPkg) const;
};
