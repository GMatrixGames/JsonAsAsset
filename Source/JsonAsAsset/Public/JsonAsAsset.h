// Copyright JAA Contributors 2024-2025

#pragma once

#include "CoreMinimal.h"

class FToolBarBuilder;
class FMenuBuilder;
class ITableRow;
class SButton;
class STableViewBase;
struct FSlateBrush;

class FJsonAsAssetModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Executes File Dialog
	void PluginButtonClicked();

private:
	void RegisterMenus();

	TSharedPtr<FUICommandList> PluginCommands;
	TSharedRef<SWidget> CreateToolbarDropdown();

	// Creates a dialog for a file
	TArray<FString> OpenFileDialog(FString Title, FString Type);
	bool IsProcessRunning(const FString& ProcessName);
};