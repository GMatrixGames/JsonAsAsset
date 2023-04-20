// Copyright Epic Games, Inc. All Rights Reserved.

#include "JsonAsAsset.h"
#include "JsonAsAssetStyle.h"
#include "JsonAsAssetCommands.h"

#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Interfaces/IMainFrameModule.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/MessageDialog.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "ToolMenus.h"
#include "LevelEditor.h"

#include "Interfaces/IPluginManager.h"
#include "Settings/JsonAsAssetSettings.h"
#include "Importers/Importer.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#include "Dialogs/Dialogs.h"
// ------------------------------------------------------ |

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

void FJsonAsAssetModule::StartupModule() {
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FJsonAsAssetStyle::Initialize();
	FJsonAsAssetStyle::ReloadTextures();

	FJsonAsAssetCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FJsonAsAssetCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJsonAsAssetModule::RegisterMenus));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	{
		TSharedPtr<FExtender> NewMenuExtender = MakeShareable(new FExtender);
		NewMenuExtender->AddMenuExtension("Tools",
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FJsonAsAssetModule::AddMenuEntry));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(NewMenuExtender);
	}

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	if (Settings->ExportDirectory.Path.IsEmpty()) {
		// Create notification
		FNotificationInfo Info(FText::FromString("Exports Directory Empty"));
		Info.ExpireDuration = 15.0f;
		Info.bUseLargeFont = true;
		Info.bUseSuccessFailIcons = true;
		Info.WidthOverride = FOptionalSize(350);
		Info.SubText = FText::FromString("For JsonAsAsset to function properly, set your exports directory in plugin settings.");
		Info.HyperlinkText = FText::FromString("JsonAsAsset");

		TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
	}
}

void FJsonAsAssetModule::ShutdownModule() {
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FJsonAsAssetStyle::Shutdown();
	FJsonAsAssetCommands::Unregister();
}

void FJsonAsAssetModule::PluginButtonClicked() {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	if (Settings->ExportDirectory.Path.IsEmpty()) {
		// Create notification
		FNotificationInfo Info(FText::FromString("Exports Directory Empty"));
		Info.ExpireDuration = 15.0f;
		Info.bUseLargeFont = true;
		Info.bUseSuccessFailIcons = true;
		Info.WidthOverride = FOptionalSize(350);
		Info.SubText = FText::FromString("For JsonAsAsset to function properly, set your exports directory in plugin settings.");
		Info.HyperlinkText = FText::FromString("JsonAsAsset");

		TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);

		return;
	}

	// Dialog for a JSON File
	TArray<FString> OutFileNames = OpenFileDialog("Open JSON files", "JSON Files|*.json");
	if (OutFileNames.Num() == 0) {
		return;
	}

	for (FString& File : OutFileNames) {
		// Import asset by IImporter
		IImporter* Importer = new IImporter();
		Importer->ImportReference(File);
	}
}

void FJsonAsAssetModule::RegisterMenus() {
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);
	
	// this places the button at the top tool bar.
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("JsonAsAsset");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitComboButton(
					"JsonAsAsset",
					FUIAction(
						FExecuteAction(),
						FCanExecuteAction(),
						FGetActionCheckState()
					),
					FOnGetContent::CreateRaw(this, &FJsonAsAssetModule::CreateToolbarMenuEntries),
					LOCTEXT("JsonAsAssetDisplayName", "JsonAsAsset"),
					LOCTEXT("JsonAsAsset", "List of actions for JsonAsAsset"),
					FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), FName("JsonAsAsset.PluginAction"))
				));

				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

TArray<FString> FJsonAsAssetModule::OpenFileDialog(FString Title, FString Type) {
	TArray<FString> ReturnValue;

	// Window Handler for Windows
	void* ParentWindowHandle = nullptr;

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();

	// Define the window handle, if it's valid
	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

		uint32 SelectionFlag = 1;
		// JSON Files|*.json
		DesktopPlatform->OpenFileDialog(ParentWindowHandle, Title, Settings->ExportDirectory.Path, FString(""), Type, SelectionFlag,
		                                ReturnValue);
	}

	return ReturnValue;
}

void FJsonAsAssetModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("JSONTools", TAttribute<FText>(FText::FromString("JSON Tools")));
	MenuBuilder.AddMenuEntry(FJsonAsAssetCommands::Get().PluginAction);
	MenuBuilder.EndSection();
}

TSharedRef<SWidget> FJsonAsAssetModule::CreateToolbarMenuEntries()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");

	FMenuBuilder MenuBuilder(false, nullptr);
	MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("JSON Tools v" + Plugin->GetDescriptor().VersionName));
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetMenu", "Asset Types"),
			LOCTEXT("JsonAsAssetMenuToolTip", "List of supported assets for JsonAsAsset"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Asset Classes"));
				{
					for (FString& Asset : IImporter::GetAcceptedTypes()) {
						InnerMenuBuilder.AddMenuEntry(
							FText::FromString(Asset),
							FText::FromString(Asset),
							FSlateIconFinder::FindCustomIconForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Asset)), TEXT("ClassThumbnail")),
							FUIAction()
						);
					}
				}
				InnerMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "LevelEditor.Tabs.Viewports")
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetButton", "Documentation"),
			LOCTEXT("JsonAsAssetButtonTooltip", "Documentation for JsonAsAsset"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "AssetEditor.Apply"),
			FUIAction(
				FExecuteAction::CreateLambda([this]() {
					FString TheURL = "https://github.com/Tectors/JsonAsAsset";
					FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
				})
			),
			NAME_None
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetButton", "JsonAsAsset"),
			LOCTEXT("JsonAsAssetButtonTooltip", "Execute JsonAsAsset"),
			FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), "JsonAsAsset.PluginAction"),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked)
			),
			NAME_None
		);
	}
	MenuBuilder.EndSection();

	/*MenuBuilder.BeginSection("JsonAsAssetSettings", FText::FromString("Settings"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetButton", "Enable Reference Automation"),
			LOCTEXT("JsonAsAssetButtonTooltip", "Recursively import references if needed"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this]() {
				})
			),
			NAME_None
		);
	}
	MenuBuilder.EndSection();*/

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FJsonAsAssetModule::FillComboButton(TSharedPtr<class FUICommandList> Commands)
{
	FMenuBuilder MenuBuilder(true, Commands);

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)
