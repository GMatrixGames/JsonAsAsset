// Copyright JAA Contributors 2024-2025

#include "./Modules/AboutJsonAsAsset.h"
#include "JsonAsAssetStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "AboutJsonAsAsset"

void SAboutJsonAsAsset::Construct(const FArguments& InArgs) {
	// Plugin Details
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");

	TSharedPtr<SButton> FModelButton;
	TSharedPtr<SButton> GithubButton;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4428)
#endif
	AboutLines.Add(MakeShareable(new FLineDefinition(LOCTEXT("JsonAsAssetDetails", "JsonAsAsset is a plugin used to convert JSON to uassets inside of the content browser. We will not be liable or responsible for activity you may do with this plugin, nor any loss or damage caused by this plugin."), 9, FLinearColor(1.f, 1.f, 1.f), FMargin(0.f, 2.f))));
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	FText Version = FText::FromString("Version: " + Plugin->GetDescriptor().VersionName);
	FText Title = FText::FromString("JsonAsAsset");

	ChildSlot
		[
			SNew(SBorder)
				.Padding(16.f).BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				[
					SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 16.f, 0.f, 0.f)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0)
								[
									SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										.Padding(0.f, 4.f)
										[
											SNew(STextBlock)
												.ColorAndOpacity(FStyleColors::ForegroundHover)
												.Font(FAppStyle::Get().GetFontStyle("AboutScreen.TitleFont"))
												.Text(Title)
										]

										+ SVerticalBox::Slot()
										.Padding(0.f, 4.f)
										[
											SNew(SEditableText)
												.IsReadOnly(true)
												.ColorAndOpacity(FStyleColors::ForegroundHover)
												.Text(Version)
										]
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Right)
								.Padding(0.0f, 0.0f, 8.f, 0.0f)
								[
									SAssignNew(FModelButton, SButton)
										.ButtonStyle(FAppStyle::Get(), "SimpleButton")
										.OnClicked(this, &SAboutJsonAsAsset::OnFModelButtonClicked)
										.ContentPadding(0.f).ToolTipText(LOCTEXT("FModelButton", "FModel Application"))
										[
											SNew(SImage)
												.Image(FJsonAsAssetStyle::Get().GetBrush(TEXT("JsonAsAsset.FModelLogo")))
												.ColorAndOpacity(FSlateColor::UseForeground())
										]
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Right)
								.Padding(0.0f, 0.0f, 8.f, 0.0f)
								[
									SAssignNew(GithubButton, SButton)
										.ButtonStyle(FAppStyle::Get(), "SimpleButton")
										.OnClicked(this, &SAboutJsonAsAsset::OnGithubButtonClicked)
										.ContentPadding(0.f).ToolTipText(LOCTEXT("GithubButton", "JsonAsAsset Github Page"))
										[
											SNew(SImage)
												.Image(FJsonAsAssetStyle::Get().GetBrush(TEXT("JsonAsAsset.GithubLogo")))
												.ColorAndOpacity(FSlateColor::UseForeground())
										]
								]
						]

						+ SVerticalBox::Slot()
						.Padding(FMargin(0.f, 16.f))
						.AutoHeight()
						[
							SNew(SListView<TSharedRef<FLineDefinition>>)
								.ListViewStyle(&FAppStyle::Get().GetWidgetStyle<FTableViewStyle>("SimpleListView"))
								.ListItemsSource(&AboutLines)
								.OnGenerateRow(this, &SAboutJsonAsAsset::MakeAboutTextItemWidget)
								.SelectionMode(ESelectionMode::None)
						]

						+ SVerticalBox::Slot()
						.Padding(FMargin(0.f, 16.f, 0.0f, 0.0f))
						.AutoHeight()
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Bottom)
						]
				]
		];
}

TSharedRef<ITableRow> SAboutJsonAsAsset::MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable) {
	if (Item->Text.IsEmpty())
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SimpleTableView.Row"))
		.Padding(6.0f)[
			SNew(SSpacer)
		];
	else
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SimpleTableView.Row"))
		.Padding(Item->Margin)[
			SNew(STextBlock)
				.LineHeightPercentage(1.3f)
				.AutoWrapText(true)
				.ColorAndOpacity(Item->TextColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", Item->FontSize))
				.Text(Item->Text)
		];
}

FReply SAboutJsonAsAsset::OnFModelButtonClicked() {
	FString TheURL = "https://fmodel.app";
	FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);

	return FReply::Handled();
}

FReply SAboutJsonAsAsset::OnGithubButtonClicked() {
	FString TheURL = "https://github.com/JsonAsAsset/JsonAsAsset";
	FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE