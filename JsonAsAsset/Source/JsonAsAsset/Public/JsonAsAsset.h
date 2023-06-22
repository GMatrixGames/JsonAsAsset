// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Layout/Margin.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

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