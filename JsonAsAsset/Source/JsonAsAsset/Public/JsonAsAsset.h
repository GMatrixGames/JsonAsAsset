// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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

	TArray<FString> OpenFileDialog(FString Title, FString Type);
	bool IsProcessRunning(const FString& ProcessName);
	TSharedRef<SWidget> CreateToolbarDropdown();

private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
