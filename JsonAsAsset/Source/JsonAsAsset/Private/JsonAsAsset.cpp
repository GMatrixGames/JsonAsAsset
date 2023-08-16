// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "JsonAsAsset.h"
#include "JsonAsAssetStyle.h"
#include "JsonAsAssetCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Interfaces/IMainFrameModule.h"
#include "LevelEditor.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include <Importer.h>
#include <Developer/DesktopPlatform/Public/DesktopPlatformModule.h>
#include <Runtime/Projects/Public/Interfaces/IPluginManager.h>
#include <Settings/JsonAsAssetSettings.h>
#include <Runtime/SlateCore/Public/Styling/SlateIconFinder.h>
#include <Developer/Settings/Public/ISettingsModule.h>
#include <Editor/EditorStyle/Public/EditorStyleSet.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef _MSC_VER
#undef GetObject
#endif

#include <TlHelp32.h>
#include <DetailLayoutBuilder.h>

static const FName JsonAsAssetTabName("JsonAsAsset");

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

#if PLATFORM_WINDOWS
static TWeakPtr<SNotificationItem> ImportantNotificationPtr;
static TWeakPtr<SNotificationItem> LocalFetchNotificationPtr;
#endif

void FJsonAsAssetModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FJsonAsAssetStyle::Initialize();
	FJsonAsAssetStyle::ReloadTextures();

	FJsonAsAssetCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FJsonAsAssetCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FJsonAsAssetModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FJsonAsAssetModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UJsonAsAssetSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FJsonAsAssetSettingsDetails::MakeInstance));
}

void FJsonAsAssetModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FJsonAsAssetStyle::Shutdown();

	FJsonAsAssetCommands::Unregister();
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
		uint32 SelectionFlag = 1;

		// Open File Dialog
		DesktopPlatform->OpenFileDialog(ParentWindowHandle, Title, FString(""), FString(""), Type, SelectionFlag, ReturnValue);
	}

	return ReturnValue;
}

bool FJsonAsAssetModule::IsProcessRunning(const FString& ProcessName) {
	bool bIsRunning = false;

	// Convert FString to WCHAR
	const TCHAR* ProcessNameChar = *ProcessName;
	const WCHAR* ProcessNameWChar = (const WCHAR*)ProcessNameChar;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 ProcessEntry;
		ProcessEntry.dwSize = sizeof(ProcessEntry);

		if (Process32First(hSnapshot, &ProcessEntry)) {
			do {
				if (_wcsicmp(ProcessEntry.szExeFile, ProcessNameWChar) == 0) {
					bIsRunning = true;
					break;
				}
			} while (Process32Next(hSnapshot, &ProcessEntry));
		}

		CloseHandle(hSnapshot);
	}

	return bIsRunning;
}

void FJsonAsAssetModule::PluginButtonClicked()
{
	// Dialog for a JSON File
	TArray<FString> OutFileNames = OpenFileDialog("Open JSON file", "JSON Files|*.json");
	if (OutFileNames.Num() == 0)
		return;

	for (FString& File : OutFileNames) {
		// Import asset by IImporter
		IImporter* Importer = new IImporter();
		Importer->ImportReference(File);
	}
}

void FJsonAsAssetModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FJsonAsAssetCommands::Get().PluginAction);
}

void FJsonAsAssetModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddComboButton(
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction(),
			FGetActionCheckState()
		),
		FOnGetContent::CreateRaw(this, &FJsonAsAssetModule::CreateToolbarDropdown),
		LOCTEXT("JsonAsAssetDisplayName", "JsonAsAsset"),
		LOCTEXT("JsonAsAsset", "List of actions for JsonAsAsset"),
		FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), FName("JsonAsAsset.PluginAction"))
	);
}


TSharedRef<SWidget> FJsonAsAssetModule::CreateToolbarDropdown() {
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	FMenuBuilder MenuBuilder(false, nullptr);
	MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("JSON Tools v" + Plugin->GetDescriptor().VersionName));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetDocButton", "Documentation"),
			LOCTEXT("JsonAsAssetDocButtonTooltip", "Documentation for JsonAsAsset"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Documentation"),
			FUIAction(
				FExecuteAction::CreateLambda([this]() {
						FString TheURL = "https://github.com/Tectors/JsonAsAsset";
						FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
					})
			),
			NAME_None
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetExButton", "JsonAsAsset"),
			LOCTEXT("JsonAsAssetExButtonTooltip", "Execute JsonAsAsset"),
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

	bool bActionRequired =
		Settings->ExportDirectory.Path.IsEmpty() //||..
		;

	if (bActionRequired) {
		MenuBuilder.BeginSection("JsonAsAssetActionRequired", FText::FromString("Action Required"));
		{
			// Export Directory Missing
			if (Settings->ExportDirectory.Path.IsEmpty())
				MenuBuilder.AddMenuEntry(
					LOCTEXT("JsonAsAssetExpoButton", "Export Directory Missing"),
					LOCTEXT("JsonAsAssetExpoButtonTooltip", "Change your exports directory in JsonAsAsset's Plugin Settings"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.WarningWithColor"),
					FUIAction(
						FExecuteAction::CreateLambda([this]() {
							// Send user to plugin
							FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
								.ShowViewer("Editor", "Plugins", "JsonAsAsset");
							})
					),
					NAME_None
								);
		}
		MenuBuilder.EndSection();
	}

	if (Settings->bEnableLocalFetch) {
		MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("Json-As-Asset API"));
		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetAsseMenu", "Asset Types"),
			LOCTEXT("JsonAsAssetAsseMenuToolTip", "List of supported classes that can be locally fetched using the API"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Asset Classes"));
				{
					TArray<FString> AcceptedTypes;
					const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

					if (Settings->bEnableLocalFetch) {
						AcceptedTypes.Add("Texture2D");
						AcceptedTypes.Add("TextureCube");
						AcceptedTypes.Add("TextureRenderTarget2D");
						AcceptedTypes.Add("CurveFloat");
						AcceptedTypes.Add("CurveLinearColor");
						AcceptedTypes.Add("CurveLinearColorAtlas");

						AcceptedTypes.Add("ReverbEffect");
						AcceptedTypes.Add("SoundAttenuation");
						AcceptedTypes.Add("SoundConcurrency");

						AcceptedTypes.Add("DataTable");
						AcceptedTypes.Add("SubsurfaceProfile");

						AcceptedTypes.Add("MaterialParameterCollection");
						AcceptedTypes.Add("MaterialFunction");
						AcceptedTypes.Add("PhysicalMaterial");
					}

					for (FString& Asset : AcceptedTypes) {
						InnerMenuBuilder.AddMenuEntry(
							FText::FromString(Asset),
							FText::FromString(Asset),
							FSlateIconFinder::FindCustomIconForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Asset)), TEXT("ClassThumbnail")),
							FUIAction()
						);

						if (Asset == "TextureRenderTarget2D" || Asset == "CurveLinearColorAtlas" || Asset == "SubsurfaceProfile") {
							InnerMenuBuilder.AddMenuSeparator();
						}
					}
				}
				InnerMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon()
		);

		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetComMenu", "Command-line Application"),
			LOCTEXT("", ""),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Console"));
				{
					InnerMenuBuilder.AddMenuEntry(
						FText::FromString("Execute JsonAsAsset API (.EXE)"),
						FText::FromString(""),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda([this]() {
								TSharedPtr<SNotificationItem> NotificationItem = LocalFetchNotificationPtr.Pin();

								if (NotificationItem.IsValid()) {
									NotificationItem->SetFadeOutDuration(0.001);
									NotificationItem->Fadeout();
									LocalFetchNotificationPtr.Reset();
								}

								FString PluginBinariesFolder;

								const TSharedPtr<IPlugin> PluginInfo = IPluginManager::Get().FindPlugin("JsonAsAsset");
								if (PluginInfo.IsValid()) {
									const FString PluginBaseDir = PluginInfo->GetBaseDir();
									PluginBinariesFolder = FPaths::Combine(PluginBaseDir, TEXT("Binaries"));

									if (!FPaths::DirectoryExists(PluginBinariesFolder)) {
										PluginBinariesFolder = FPaths::Combine(PluginBaseDir, TEXT("JsonAsAsset/Binaries"));
									}
								}

								FString FullPath = FPaths::ConvertRelativePathToFull(PluginBinariesFolder + "/Win64/JsonAsAsset_API/JsonAsAssetAPI.exe");
								FPlatformProcess::LaunchFileInDefaultExternalApplication(*FullPath, TEXT("--urls=http://localhost:1500/"), ELaunchVerb::Open);
								}),
							FCanExecuteAction::CreateLambda([this]() {
									return !IsProcessRunning("JsonAsAssetAPI.exe");
								})
									)
					);

					InnerMenuBuilder.AddMenuEntry(
						FText::FromString("Shutdown JsonAsAsset API (.EXE)"),
						FText::FromString(""),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda([this]() {
								FString ProcessName = TEXT("JsonAsAssetAPI.exe");
								TCHAR* ProcessNameChar = ProcessName.GetCharArray().GetData();

								DWORD ProcessID = 0;
								HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
								if (hSnapshot != INVALID_HANDLE_VALUE) {
									PROCESSENTRY32 ProcessEntry;
									ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

									if (Process32First(hSnapshot, &ProcessEntry)) {
										do {
											if (FCString::Stricmp(ProcessEntry.szExeFile, ProcessNameChar) == 0) {
												ProcessID = ProcessEntry.th32ProcessID;
												break;
											}
										} while (Process32Next(hSnapshot, &ProcessEntry));
									}
									CloseHandle(hSnapshot);
								}

								if (ProcessID != 0) {
									HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, ProcessID);
									if (hProcess != nullptr) {
										TerminateProcess(hProcess, 0);
										CloseHandle(hProcess);
									}
								}
								}),
							FCanExecuteAction::CreateLambda([this]() {
									return IsProcessRunning("JsonAsAssetAPI.exe");
								})
									)
					);
				}
				InnerMenuBuilder.EndSection();
				}),
			false,
					FSlateIcon()
					);
		MenuBuilder.EndSection();
	}

	MenuBuilder.AddMenuSeparator();
	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetPluButton", "Open Plugin Settings"),
		LOCTEXT("JsonAsAssetPluButtonTooltip", "Brings you to the JsonAsAsset Settings"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Settings"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() {
				// Send user to plugin
				FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
					.ShowViewer("Editor", "Plugins", "JsonAsAsset");
				})
		),
		NAME_None
					);

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)