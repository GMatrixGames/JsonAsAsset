// Copyright JAA Contributors 2024-2025

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Layout/Margin.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

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
			Margin(InMargin) {}
	};

	TSharedRef<ITableRow> MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable);
	FReply OnFModelButtonClicked();
	FReply OnGithubButtonClicked();
};