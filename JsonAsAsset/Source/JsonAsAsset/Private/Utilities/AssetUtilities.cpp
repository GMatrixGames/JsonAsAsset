// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/AssetUtilities.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Settings/JsonAsAssetSettings.h"
#include "Dom/JsonObject.h"

#include "UObject/SavePackage.h"

#include "HttpModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Importers/TextureImporter.h"
#include "Importers/MaterialParameterCollectionImporter.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/RemoteUtilities.h"
#include "PluginUtils.h"

UPackage* FAssetUtilities::CreateAssetPackage(const FString& FullPath) {
	UPackage* Package = CreatePackage(*FullPath);
	UPackage* _ = Package->GetOutermost(); // ??
	Package->FullyLoad();

	return Package;
}

UPackage* FAssetUtilities::CreateAssetPackage(const FString& Name, const FString& OutputPath) {
	UPackage* Ignore = nullptr;
	return CreateAssetPackage(Name, OutputPath, Ignore);
}

UPackage* FAssetUtilities::CreateAssetPackage(const FString& Name, const FString& OutputPath, UPackage*& OutOutermostPkg) {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	FString ModifiablePath;

	// References Automatically Formatted
	if ((!OutputPath.StartsWith("/Game/") && !OutputPath.StartsWith("/Plugins/")) && OutputPath.Contains("Content")) {
		OutputPath.Split(*(Settings->ExportDirectory.Path + "/"), nullptr, &ModifiablePath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		ModifiablePath.Split("/", nullptr, &ModifiablePath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		ModifiablePath.Split("/", &ModifiablePath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		// Ex: RestPath: Plugins/ContentLibraries/EpicBaseTextures
		// Ex: RestPath: Content/Athena
		bool bIsPlugin = ModifiablePath.StartsWith("Plugins");

		if (Settings->bEnableModifications) {
			if (bIsPlugin) {
				FString PluginName = ModifiablePath;
				FString RemaningPath;
				// Plugins/GameFeatures/Creative/CRP/CRP_Sunburst/Content/SetupAssets/Materials
				// Plugins/GameFeatures/Creative/CRP/CRP_Sunburst
				// PluginName = CRP_Sunburst
				// RemaningPath = SetupAssets/Materials
				PluginName.Split("/Content/", &PluginName, &RemaningPath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				PluginName.Split("/", nullptr, &PluginName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

				// /CRP_Sunburst/SetupAssets/Materials
				ModifiablePath = PluginName + "/" + RemaningPath;
				ModifiablePath = Settings->RedirectFolderDirectory.Path.Replace(TEXT("/Game/"), TEXT("Game/")) + ModifiablePath;
			} else {
				FString DirectString;
				Settings->RedirectFolderDirectory.Path.Split("/", &DirectString, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				ModifiablePath = ModifiablePath.Replace(TEXT("Content"), *(DirectString.Replace(TEXT("/Game/"), TEXT("Game/"))));
			}
		} else {
			// Plugins/ContentLibraries/EpicBaseTextures -> ContentLibraries/EpicBaseTextures
			if (bIsPlugin) {
				FString PluginName = ModifiablePath;
				FString RemaningPath;
				// Plugins/GameFeatures/Creative/CRP/CRP_Sunburst/Content/SetupAssets/Materials
				// Plugins/GameFeatures/Creative/CRP/CRP_Sunburst
				// PluginName = CRP_Sunburst
				// RemaningPath = SetupAssets/Materials
				PluginName.Split("/Content/", &PluginName, &RemaningPath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				PluginName.Split("/", nullptr, &PluginName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

				// /CRP_Sunburst/SetupAssets/Materials
				ModifiablePath = PluginName + "/" + RemaningPath;

				if (Settings->bEnableModifications)
					ModifiablePath = Settings->RedirectFolderDirectory.Path.Replace(TEXT("/Game/"), TEXT("Game/")) + "Plugins" + ModifiablePath;
			}
			// Content/Athena -> Game/Athena
			else ModifiablePath = ModifiablePath.Replace(TEXT("Content"), TEXT("Game"));
		}

		// ContentLibraries/EpicBaseTextures -> /ContentLibraries/EpicBaseTextures/
		ModifiablePath = "/" + ModifiablePath + "/";

		// Check if plugin exists
		if (bIsPlugin && !Settings->bEnableModifications) {
			FString PluginName;
			ModifiablePath.Split("/", nullptr, &PluginName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			PluginName.Split("/", &PluginName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);

			if (IPluginManager::Get().FindPlugin(PluginName) == nullptr)
				CreatePlugin(PluginName);
		}
	} else {
		FString RootName; {
			OutputPath.Split("/", nullptr, &RootName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			RootName.Split("/", &RootName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		}

		if (!Settings->bEnableModifications)
			if ((RootName != "Game" && RootName != "Engine") && IPluginManager::Get().FindPlugin(RootName) == nullptr)
				CreatePlugin(RootName);

		ModifiablePath = OutputPath;
		ModifiablePath.Split("/", &ModifiablePath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		ModifiablePath = ModifiablePath + "/";

		FString DirectString;
		Settings->RedirectFolderDirectory.Path.Split("/", &DirectString, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		if ((RootName != "Game" && RootName != "Engine") && Settings->bEnableModifications)
			ModifiablePath = DirectString + "/Plugins" + ModifiablePath;
		else if (Settings->bEnableModifications) 
			ModifiablePath = ModifiablePath.Replace(TEXT("Game"), *(DirectString.Replace(TEXT("/Game/"), TEXT("Game/"))));
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

		return nullptr;
	}

	return SelectedAssets[0].GetAsset();
}

// Constructing assets ect..
template <typename T>
bool FAssetUtilities::ConstructAsset(const FString& Path, const FString& Type, TObjectPtr<T>& OutObject, bool& bSuccess) {
	// Supported Assets
	if (Type == "Texture2D" ||
		// Type == "TextureCube" ||
		// Type == "VolumeTexture" ||
		Type == "TextureRenderTarget2D" ||
		Type == "MaterialParameterCollection" ||
		Type == "CurveFloat" ||
		Type == "CurveTable" ||
		Type == "CurveVector" ||
		Type == "CurveLinearColorAtlas" ||
		Type == "CurveLinearColor" ||
		Type == "PhysicalMaterial" ||
		Type == "SubsurfaceProfile" ||
		Type == "LandscapeGrassType" ||
		Type == "MaterialInstanceConstant" ||
		Type == "ReverbEffect" ||
		Type == "SoundAttenuation" ||
		Type == "SoundConcurrency" ||
		Type == "DataTable" ||
		Type == "SubsurfaceProfile" ||
		Type == "MaterialFunction" ||
		Type == "Material" // might cause issues
	) {
		//		Manually supported asset types
		// (ex: textures have to be handled separately)
		if (Type ==
			"Texture2D" ||
			Type == "TextureRenderTarget2D" ||
			Type == "TextureCube" ||
			Type == "VolumeTexture"
			) {
			UTexture* Texture;
			FString NewPath = Path;

			FString RootName; {
				NewPath.Split("/", nullptr, &RootName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				RootName.Split("/", &RootName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			}

			const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

			FString DirectString;
			Settings->RedirectFolderDirectory.Path.Split("/", &DirectString, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

			// Missing Plugin: Create it
			if (Settings->bEnableModifications)
				// Missing Plugin: Change Reference To /Game/Plugins/...
				if (RootName != "Game" && RootName != "Engine") {
					DirectString = Settings->RedirectFolderDirectory.Path;

					NewPath = DirectString + "Plugins" + NewPath;
				} else NewPath = Settings->RedirectFolderDirectory.Path + NewPath.Replace(TEXT("/Game/"), TEXT(""));
			else if (RootName != "Game" && RootName != "Engine" && IPluginManager::Get().FindPlugin(RootName) == nullptr)
				CreatePlugin(RootName);

			bSuccess = Construct_TypeTexture(NewPath, Path, Texture);
			if (bSuccess) OutObject = Cast<T>(Texture);

			return true;
		} else {
			const TSharedPtr<FJsonObject> Response = API_RequestExports(Path);
			if (Response == nullptr || Path.IsEmpty()) return true;

			TSharedPtr<FJsonObject> JsonObject = Response->GetArrayField("jsonOutput")[0]->AsObject();
			FString PackagePath;
			FString AssetName;
			Path.Split(".", &PackagePath, &AssetName);

			if (JsonObject) {
				FString NewPath = PackagePath;

				const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

				FString RootName; {
					NewPath.Split("/", nullptr, &RootName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
					RootName.Split("/", &RootName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				}

				FString DirectString;
				Settings->RedirectFolderDirectory.Path.Split("/", &DirectString, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

				if (Settings->bEnableModifications)
					if (RootName != "Game" && RootName != "Engine") {
						DirectString = Settings->RedirectFolderDirectory.Path;

						NewPath = DirectString + "Plugins" + NewPath;
					}
					else NewPath = Settings->RedirectFolderDirectory.Path + NewPath.Replace(TEXT("/Game/"), TEXT(""));
				else if (RootName != "Game" && RootName != "Engine" && IPluginManager::Get().FindPlugin(RootName) == nullptr)
					CreatePlugin(RootName);

				UPackage* OutermostPkg;
				UPackage* Package = CreatePackage(*NewPath);
				OutermostPkg = Package->GetOutermost();
				Package->FullyLoad();

				// Import asset by IImporter
				IImporter* Importer = new IImporter();
				bSuccess = Importer->HandleExports(Response->GetArrayField("jsonOutput"), PackagePath, true);

				// Define found object
				OutObject = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *Path));

				return true;
			}
		}
	}

	return false;
}

bool FAssetUtilities::Construct_TypeTexture(const FString& Path, const FString& RealPath, UTexture*& OutTexture) {
	if (Path.IsEmpty()) 
		return false;

	TSharedPtr<FJsonObject> JsonObject = API_RequestExports(RealPath);
	if (JsonObject == nullptr)
		return false;

	TArray<TSharedPtr<FJsonValue>> Response = JsonObject->GetArrayField("jsonOutput");
	if (Response.IsEmpty())
		return false;

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	TSharedPtr<FJsonObject> JsonExport = Response[0]->AsObject();
	FString Type = JsonExport->GetStringField("Type");
	UTexture* Texture = nullptr;
	TArray<uint8> Data = TArray<uint8>();

	// --------------- Download Texture Data ------------
	if (Type != "TextureRenderTarget2D") {
		FHttpModule* HttpModule = &FHttpModule::Get();
		const TSharedRef<IHttpRequest> HttpRequest = HttpModule->CreateRequest();

		HttpRequest->SetURL(Settings->Url + "/api/v1/export?path=" + RealPath);
		HttpRequest->SetHeader("content-type", "application/octet-stream");
		HttpRequest->SetVerb(TEXT("GET"));

		const TSharedPtr<IHttpResponse> HttpResponse = FRemoteUtilities::ExecuteRequestSync(HttpRequest);
		if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
			return false;

		if (HttpResponse->GetContentType().StartsWith("application/json; charset=utf-8")) {
			return false;
		}

		Data = HttpResponse->GetContent();
		if (Data.Num() == 0)
			return false;
	}

	FString PackagePath; FString AssetName; {
		Path.Split(".", &PackagePath, &AssetName);
	}

	UPackage* Package = CreatePackage(*PackagePath);
	UPackage* OutermostPkg = Package->GetOutermost();
	Package->FullyLoad();

	// Create Importer
	const UTextureImporter* Importer = new UTextureImporter(AssetName, Path, Response[0]->AsObject(), Package, OutermostPkg);

	if (Type == "Texture2D")
		Importer->ImportTexture2D(Texture, Data, JsonExport);
	if (Type == "TextureCube")
		Importer->ImportTextureCube(Texture, Data, JsonExport);
	if (Type == "VolumeTexture")
		Importer->ImportVolumeTexture(Texture, Data, JsonExport);
	if (Type == "TextureRenderTarget2D")
		Importer->ImportRenderTarget2D(Texture, JsonExport->GetObjectField("Properties"));

	if (Texture == nullptr)
		return false;

	FAssetRegistryModule::AssetCreated(Texture);
	if (!Texture->MarkPackageDirty())
		return false;

	Package->SetDirtyFlag(true);
	Texture->PostEditChange();
	Texture->AddToRoot();
	Package->FullyLoad();

	// Save texture
	if (Settings->bAllowPackageSaving) {
		FSavePackageArgs SaveArgs; {
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
		}

		const FString PackageName = Package->GetName();
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
	}

	OutTexture = Texture;

	return true;
}

void FAssetUtilities::CreatePlugin(FString PluginName) {
	FPluginUtils::FNewPluginParamsWithDescriptor CreationParams;
	CreationParams.Descriptor.bCanContainContent = true;

	CreationParams.Descriptor.FriendlyName = PluginName;
	CreationParams.Descriptor.Version = 1;
	CreationParams.Descriptor.VersionName = TEXT("1.0");
	CreationParams.Descriptor.Category = TEXT("Other");

	FText FailReason;
	FPluginUtils::FLoadPluginParams LoadParams;
	LoadParams.bEnablePluginInProject = true;
	LoadParams.bUpdateProjectPluginSearchPath = true;
	LoadParams.bSelectInContentBrowser = false;

	FPluginUtils::CreateAndLoadNewPlugin(PluginName, FPaths::ProjectPluginsDir(), CreationParams, LoadParams);

#define LOCTEXT_NAMESPACE "UMG"
#if WITH_EDITOR
	// Setup notification's arguments
	FFormatNamedArguments Args;
	Args.Add(TEXT("PluginName"), FText::FromString(PluginName));

	// Create notification
	FNotificationInfo Info(FText::Format(LOCTEXT("PluginCreated", "Plugin Created: {PluginName}"), Args));
	Info.ExpireDuration = 10.0f;
	Info.bUseLargeFont = true;
	Info.bUseSuccessFailIcons = false;
	Info.WidthOverride = FOptionalSize(350);
	Info.SubText = FText::FromString(FString("Created successfully"));

	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(SNotificationItem::CS_Success);
#endif
#undef LOCTEXT_NAMESPACE
}

const TSharedPtr<FJsonObject> FAssetUtilities::API_RequestExports(const FString& Path) {
	FHttpModule* HttpModule = &FHttpModule::Get();
	const TSharedRef<IHttpRequest> HttpRequest = HttpModule->CreateRequest();

	FString PackagePath;
	FString AssetName;
	Path.Split(".", &PackagePath, &AssetName);

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	const TSharedRef<IHttpRequest> NewRequest = HttpModule->CreateRequest();
	NewRequest->SetURL(Settings->Url + "/api/v1/export?raw=true&path=" + Path);
	NewRequest->SetVerb(TEXT("GET"));

	const TSharedPtr<IHttpResponse> NewResponse = FRemoteUtilities::ExecuteRequestSync(NewRequest);
	if (!NewResponse.IsValid()) return TSharedPtr<FJsonObject>();

	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(NewResponse->GetContentAsString());
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
		return JsonObject;

	return TSharedPtr<FJsonObject>();
}