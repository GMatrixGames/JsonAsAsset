// Copyright Epic Games, Inc. All Rights Reserved.

#include "JsonAsAsset.h"
#include "JsonAsAssetStyle.h"
#include "JsonAsAssetCommands.h"

// ------------------------------------------------------ |
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/MessageDialog.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "ToolMenus.h"
#include "LevelEditor.h"

#include "Windows/MinWindows.h"

#include "Interfaces/IPluginManager.h"
#include "Settings/JsonAsAssetSettings.h"
#include "Importers/Importer.h"

#include "HttpModule.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IMessageLogListing.h"

#include "Dialogs/Dialogs.h"
#include "ISettingsModule.h"

#include "MessageLog/Public/MessageLogInitializationOptions.h"
#include "MessageLog/Public/MessageLogModule.h"
#include "Utilities/RemoteUtilities.h"

#include "Importers/MaterialFunctionImporter.h"

#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersion.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Styling/CoreStyle.h"
#include <TlHelp32.h>

#include "Interfaces/IMainFrameModule.h"
#include "Logging/MessageLog.h"

#ifdef _MSC_VER
#undef GetObject
#endif

// ------------------------------------------------------ |

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

#if PLATFORM_WINDOWS
static TWeakPtr<SNotificationItem> ImportantNotificationPtr;
static TWeakPtr<SNotificationItem> LocalFetchNotificationPtr;
#endif

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

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	if (Settings->ExportDirectory.Path.IsEmpty()) {
		const FText TitleText = LOCTEXT("JsonAsAssetNotificationTitle", "Missing export directory for JsonAsAsset");
		const FText MessageText = LOCTEXT("JsonAsAssetNotificationText",
			"JsonAsAsset requires an export directory to handle references and to locally check for files to import. The plugin may not function properly without this set.\n\nFor more information, please see the documentation for JsonAsAsset."
		);

		FNotificationInfo Info(TitleText);

		Info.HyperlinkText = LOCTEXT("UnrealSoftwareRequirements", "JsonAsAsset Docs");
		Info.Hyperlink = FSimpleDelegate::CreateStatic([]() { 
			FString TheURL = "https://github.com/Tectors/JsonAsAsset";
			FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr); 
		});

		Info.bFireAndForget = false;
		Info.FadeOutDuration = 3.0f;
		Info.ExpireDuration = 0.0f;
		Info.bUseLargeFont = false;
		Info.bUseThrobber = false;

		Info.ButtonDetails.Add(
			FNotificationButtonInfo(LOCTEXT("OpenPluginSettings", "Open Settings"), FText::GetEmpty(),
				FSimpleDelegate::CreateStatic([]() {
					TSharedPtr<SNotificationItem> NotificationItem = ImportantNotificationPtr.Pin();

					if (NotificationItem.IsValid())
					{
						NotificationItem->Fadeout();
						ImportantNotificationPtr.Reset();
					}

					// Send user to plugin
					FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
						.ShowViewer("Editor", "Plugins", "JsonAsAsset");
				})
			)
		);

		ImportantNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		ImportantNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}

	// Message Log
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		FMessageLogInitializationOptions InitOptions;
		InitOptions.bShowPages = true;
		InitOptions.bAllowClear = true;
		InitOptions.bShowFilters = true;
		MessageLogModule.RegisterLogListing("JsonAsAsset", NSLOCTEXT("JsonAsAsset", "JsonAsAssetLogLabel", "JsonAsAsset"), InitOptions);
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UJsonAsAssetSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FJsonAsAssetSettingsDetails::MakeInstance));
}

void FJsonAsAssetModule::ShutdownModule() {
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FJsonAsAssetStyle::Shutdown();
	FJsonAsAssetCommands::Unregister();

	if (FModuleManager::Get().IsModuleLoaded("MessageLog")) {
		FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing("JsonAsAsset");
	}
}

void FJsonAsAssetModule::PluginButtonClicked() {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	if (Settings->ExportDirectory.Path.IsEmpty())
		return;

	if (Settings->bEnableLocalFetch) {
		TSharedPtr<SNotificationItem> NotificationItem = LocalFetchNotificationPtr.Pin();

		if (NotificationItem.IsValid()) {
			NotificationItem->SetFadeOutDuration(0.001);
			NotificationItem->Fadeout();
			LocalFetchNotificationPtr.Reset();
		}

		bool bIsLocalHost = Settings->Url.StartsWith("http://localhost");

		if (!IsProcessRunning("JsonAsAssetAPI.exe") && bIsLocalHost) {
			FNotificationInfo Info(LOCTEXT("JsonAsAssetNotificationTitle", "Local Fetch API"));

			Info.HyperlinkText = LOCTEXT("UnrealSoftwareRequirements", "JsonAsAsset Docs");
			Info.Hyperlink = FSimpleDelegate::CreateStatic([]() {
				FString TheURL = "https://github.com/Tectors/JsonAsAsset";
				FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
			});

			Info.bFireAndForget = false;
			Info.FadeOutDuration = 3.0f;
			Info.ExpireDuration = 3.0f;
			Info.bUseLargeFont = false;
			Info.bUseThrobber = false;
			Info.Image = FJsonAsAssetStyle::Get().GetBrush("JsonAsAsset.PluginAction");

			Info.ButtonDetails.Add(
				FNotificationButtonInfo(LOCTEXT("StartLocalFetch", "Execute JsonAsAsset API (.EXE)"), FText::GetEmpty(),
					FSimpleDelegate::CreateStatic([]() {
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
					})
				)
			);

			LocalFetchNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
			LocalFetchNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);

			return;
		}
	}

	// Dialog for a JSON File
	TArray<FString> OutFileNames = OpenFileDialog("Open JSON file", "JSON Files|*.json");
	if (OutFileNames.Num() == 0)
		return;

	for (FString& File : OutFileNames) {
		// Clear Message Log
		FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		TSharedRef<IMessageLogListing> LogListing = (MessageLogModule.GetLogListing("JsonAsAsset"));
		LogListing->ClearMessages();

		// Import asset by IImporter
		IImporter* Importer = new IImporter();
		Importer->ImportReference(File);
	}
}

void FJsonAsAssetModule::RegisterMenus() {
	FToolMenuOwnerScoped OwnerScoped(this);

	// Register JsonAsAsset toolbar dropdown button
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("JsonAsAsset");

	FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitComboButton(
		"JsonAsAsset",
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction(),
			FGetActionCheckState()
		),
		FOnGetContent::CreateRaw(this, &FJsonAsAssetModule::CreateToolbarDropdown),
		LOCTEXT("JsonAsAssetDisplayName", "JsonAsAsset"),
		LOCTEXT("JsonAsAsset", "List of actions for JsonAsAsset"),
		FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), FName("JsonAsAsset.PluginAction"))
	));

	Entry.SetCommandList(PluginCommands);
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

TSharedRef<SWidget> FJsonAsAssetModule::CreateToolbarDropdown() {
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	FMenuBuilder MenuBuilder(false, nullptr);
	MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("JSON Tools v" + Plugin->GetDescriptor().VersionName));
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetTypes", "Asset Types"),
			LOCTEXT("JsonAsAssetTypesMenuToolTip", "List of supported assets for JsonAsAsset"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetAssetTypeSection", LOCTEXT("JsonAsAssetAssetTypeSection", "Asset Classes"));
				{
					for (FString& Asset : IImporter::GetAcceptedTypes()) {
						InnerMenuBuilder.AddMenuEntry(
							FText::FromString(Asset),
							FText::FromString(Asset),
							FSlateIconFinder::FindCustomIconForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Asset)), TEXT("ClassThumbnail")),
							FUIAction()
						);

						if (Asset == "CurveLinearColorAtlas" || Asset == "Skeleton" || Asset == "AnimMontage" || Asset == "NiagaraParameterCollection" || Asset == "LandscapeGrassType" || Asset == "SoundConcurrency" || Asset == "SubsurfaceProfile") {
							InnerMenuBuilder.AddSeparator();
						}
					}
				}
				InnerMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports")
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetDocumentationButton", "Documentation"),
			LOCTEXT("JsonAsAssetDocumentationButtonTooltip", "Documentation for JsonAsAsset"),
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

	bool bActionRequired =
		Settings->ExportDirectory.Path.IsEmpty() //||..
		;

	if (bActionRequired) {
		MenuBuilder.BeginSection("JsonAsAssetActionRequired", FText::FromString("Action Required"));
		{
			// Export Directory Missing
			if (Settings->ExportDirectory.Path.IsEmpty())
				MenuBuilder.AddMenuEntry(
					LOCTEXT("JsonAsAssetExportDirectoryButton", "Export Directory Missing"),
					LOCTEXT("JsonAsAssetExportDirectoryButtonTooltip", "Change your exports directory in JsonAsAsset's Plugin Settings"),
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
		MenuBuilder.BeginSection("JsonAsAssetAPISection", FText::FromString("Json-As-Asset API"));
		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetAPIAssetTypesMenu", "Asset Types"),
			LOCTEXT("JsonAsAssetAPIAssetTypesMenuToolTip", "List of supported classes that can be locally fetched using the API"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetClassesSection", LOCTEXT("JsonAsAssetClassesSection", "Asset Classes"));
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
							InnerMenuBuilder.AddSeparator();
						}
					}
				}
				InnerMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon()
		);

		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetCommandLineMenu", "Command-line Application"),
			LOCTEXT("", ""),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetCommandLineSection", LOCTEXT("JsonAsAssetCommandLineSection", "Console"));
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

	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetPluginSettingsButton", "Open Plugin Settings"),
		LOCTEXT("JsonAsAssetPluginSettingsButtonTooltip", "Brings you to the JsonAsAsset Settings"),
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
	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetMessageLogButton", "Open Message Log"),
		LOCTEXT("JsonAsAssetMessageLogButtonTooltip", "Message Log for JsonAsAsset\n> Allows you to see what went wrong, and what went great"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "MessageLog.TabIcon"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() {
				FMessageLog MessageLogger = FMessageLog(FName("JsonAsAsset"));
				MessageLogger.Open(EMessageSeverity::Info, true);
			})
		),
		NAME_None
	);

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)
