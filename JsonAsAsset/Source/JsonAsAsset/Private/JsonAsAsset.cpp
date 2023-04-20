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
#include "ISettingsModule.h"
// ------------------------------------------------------ |

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

void FJsonAsAssetModule::StartupModule() {
	FJsonAsAssetStyle::Initialize();
	FJsonAsAssetStyle::ReloadTextures();
	FJsonAsAssetCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FJsonAsAssetCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJsonAsAssetModule::RegisterMenus));

	/*FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	{
		TSharedPtr<FExtender> NewMenuExtender = MakeShareable(new FExtender);
		NewMenuExtender->AddMenuExtension("Tools",
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FJsonAsAssetModule::AddMenuEntry));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(NewMenuExtender);
	}*/
}

void FJsonAsAssetModule::ShutdownModule() {
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FJsonAsAssetStyle::Shutdown();
	FJsonAsAssetCommands::Unregister();
}

void FJsonAsAssetModule::PluginButtonClicked() {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	if (Settings->ExportDirectory.Path.IsEmpty())
		return;

	// Dialog for a JSON File
	TArray<FString> OutFileNames = OpenFileDialog("Open JSON file", "JSON Files|*.json");
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

// NOTE: Unused
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
				FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
				FCanExecuteAction::CreateLambda([this]() {
					const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

					return !Settings->ExportDirectory.Path.IsEmpty();
				})
			),
			NAME_None
		);
	}
	MenuBuilder.EndSection();

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	bool bActionRequired =
		Settings->ExportDirectory.Path.IsEmpty() //||..
		;

	if (bActionRequired) {
		MenuBuilder.BeginSection("JsonAsAssetActionRequired", FText::FromString("Action Required"));
		{
			// Export Directory Missing
			if (Settings->ExportDirectory.Path.IsEmpty())
				MenuBuilder.AddMenuEntry(
					LOCTEXT("JsonAsAssetButton", "Export Directory Missing"),
					LOCTEXT("JsonAsAssetButtonTooltip", "Change your exports directory in JsonAsAsset's Plugin Settings"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.WarningWithColor"),
					FUIAction(
						FExecuteAction::CreateLambda([this]() {
							// Send user to plugin
							FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
								.ShowViewer("Project", "Plugins", "JsonAsAsset");
						})
					),
					NAME_None
				);
		}
		MenuBuilder.EndSection();
	}

	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetButton", "Open Plugin Settings"),
		LOCTEXT("JsonAsAssetButtonTooltip", "Brings you to the JsonAsAsset Settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() {
				// Send user to plugin
				FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
					.ShowViewer("Project", "Plugins", "JsonAsAsset");
			})
		),
		NAME_None
	);

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FJsonAsAssetModule::FillComboButton(TSharedPtr<class FUICommandList> Commands)
{
	FMenuBuilder MenuBuilder(true, Commands);

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)
