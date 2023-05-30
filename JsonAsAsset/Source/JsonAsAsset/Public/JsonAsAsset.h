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

// About JsonAsAsset (copied from Epic Games Source)
class SAboutJsonAsAsset : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SAboutJsonAsAsset) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FLineDefinition {
		FText Text;
		int32 FontSize;
		FLinearColor TextColor;
		FMargin Margin;

		FLineDefinition(const FText& InText)
			: Text(InText),
			  FontSize(9),
			  TextColor(FLinearColor(0.5f, 0.5f, 0.5f)),
			  Margin(FMargin(6.f, 0.f, 0.f, 0.f)) {
		}

		FLineDefinition(const FText& InText, int32 InFontSize, const FLinearColor& InTextColor, const FMargin& InMargin)
			: Text(InText),
			  FontSize(InFontSize),
			  TextColor(InTextColor),
			  Margin(InMargin) { }
	};

	TArray<TSharedRef<FLineDefinition>> AboutLines;
	TSharedPtr<SButton> FModelButton;
	TSharedPtr<SButton> GithubButton;

	TSharedRef<ITableRow> MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable);
	FReply OnFModelButtonClicked();
	FReply OnGithubButtonClicked();
};
