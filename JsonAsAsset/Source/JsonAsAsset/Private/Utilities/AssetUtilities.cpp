// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/AssetUtilities.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Settings/JsonAsAssetSettings.h"
#include "Dom/JsonObject.h"

UPackage* FAssetUtilities::CreateAssetPackage(const FString& FullPath) {
	UPackage* Package = CreatePackage(*FullPath);
	UPackage* _ = Package->GetOutermost();
	Package->FullyLoad();

	GLog->Log(FullPath);

	return Package;
}

UPackage* FAssetUtilities::CreateAssetPackage(const FString& Name, const FString& OutputPath) {
	UPackage* Ignore = nullptr;
	return CreateAssetPackage(Name, OutputPath, Ignore);
}

UPackage* FAssetUtilities::CreateAssetPackage(const FString& Name, const FString& OutputPath, UPackage*& OutOutermostPkg) {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	// Ex: FortniteGame/..../.../ -> /..../.../
	FString ModifiablePath;
	OutputPath.Split(*(Settings->ExportDirectory.Path + "/"), nullptr, &ModifiablePath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	ModifiablePath.Split("/", nullptr, &ModifiablePath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	ModifiablePath.Split("/", &ModifiablePath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	// Ex: RestPath: Plugins/ContentLibraries/EpicBaseTextures
	// Ex: RestPath: Content/Athena
	bool bIsPlugin = ModifiablePath.StartsWith("Plugins");

	// Plugins/ContentLibraries/EpicBaseTextures -> ContentLibraries/EpicBaseTextures
	if (bIsPlugin) ModifiablePath = ModifiablePath.Replace(TEXT("Plugins/"), TEXT("")).Replace(TEXT("GameFeatures/"), TEXT("")).Replace(TEXT("Content/"), TEXT(""));
	// Content/Athena -> Game/Athena
	else ModifiablePath = ModifiablePath.Replace(TEXT("Content"), TEXT("Game"));

	// ContentLibraries/EpicBaseTextures -> /ContentLibraries/EpicBaseTextures/
	ModifiablePath = "/" + ModifiablePath + "/";

	// Check if plugin exists
	if (bIsPlugin) {
		FString PluginName;
		ModifiablePath.Split("/", nullptr, &PluginName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		PluginName.Split("/", &PluginName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);

		if (IPluginManager::Get().FindPlugin(PluginName) == nullptr) {
			#define LOCTEXT_NAMESPACE "UMG"
			#if WITH_EDITOR
			// Setup notification's arguments
			FFormatNamedArguments Args;
			Args.Add(TEXT("PluginName"), FText::FromString(PluginName));

			// Create notification
			FNotificationInfo Info(FText::Format(LOCTEXT("NeedPlugin", "Plugin Missing: {PluginName}"), Args));
			Info.ExpireDuration = 10.0f;
			Info.bUseLargeFont = true;
			Info.bUseSuccessFailIcons = true;
			Info.WidthOverride = FOptionalSize(350);
			Info.SubText = FText::FromString(FString("Asset will be placed in Content Folder"));

			TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
			NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
			#endif
			#undef LOCTEXT_NAMESPACE

			ModifiablePath = FString(TEXT("/Game/"));
		}
	}

	const FString PathWithGame = ModifiablePath + Name;
	UPackage* Package = CreatePackage(*PathWithGame);
	OutOutermostPkg = Package->GetOutermost();
	Package->FullyLoad();

	return Package;
}

UObject* FAssetUtilities::GetSelectedAsset() {
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.Num() == 0) {
		GLog->Log("JsonAsAsset: [GetSelectedAsset] None selected, returning nullptr.");

		const FText DialogText = FText::FromString(TEXT("A function to find a selected asset failed, please select a asset to go further."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		// None found, therefore we need to return nullptr.
		return nullptr;
	}

	// Return only the first selected asset
	return SelectedAssets[0].GetAsset();
}

FRichCurveKey FAssetUtilities::ObjectToRichCurveKey(const TSharedPtr<FJsonObject>& Object) {
	FString InterpMode = Object->GetStringField("InterpMode");
	return FRichCurveKey(Object->GetNumberField("Time"), Object->GetNumberField("Value"), Object->GetNumberField("ArriveTangent"), Object->GetNumberField("LeaveTangent"), static_cast<ERichCurveInterpMode>(StaticEnum<ERichCurveInterpMode>()->GetValueByNameString(InterpMode)));
}
