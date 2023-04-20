// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct ImportOptionsBase {
	bool bImportWithPath;
};

class FToolBarBuilder;
class FMenuBuilder;

class FJsonAsAssetModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Executes File Dialog
	void PluginButtonClicked();
private:
	void RegisterMenus();

	TSharedPtr<FUICommandList> PluginCommands;

	// UI Functions
	void AddMenuEntry(FMenuBuilder& MenuBuilder);
	TSharedRef<SWidget> FillComboButton(TSharedPtr<class FUICommandList> Commands);
	TSharedRef<SWidget> CreateToolbarMenuEntries();
	
	// Creates a dialog for a file
	TArray<FString> OpenFileDialog(FString Title, FString Type);
};
