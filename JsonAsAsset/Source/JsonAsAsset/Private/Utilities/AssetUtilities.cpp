// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/AssetUtilities.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Settings/JsonAsAssetSettings.h"
#include "Dom/JsonObject.h"

#include "HttpModule.h"
#include "AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Importers/TextureImporter.h"
#include "Importers/MaterialParameterCollectionImporter.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/RemoteUtilities.h"

UPackage* FAssetUtilities::CreateAssetPackage(const FString& FullPath) {
	UPackage* Package = CreatePackage(nullptr, *FullPath);
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

			if (IPluginManager::Get().FindPlugin(PluginName).Get() == nullptr)
				CreatePlugin(PluginName);
		}
	}
	else {
		FString RootName; {
			OutputPath.Split("/", nullptr, &RootName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			RootName.Split("/", &RootName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		}

		// Missing Plugin: Create it
		if (RootName != "Game" && RootName != "Engine" && IPluginManager::Get().FindPlugin(RootName).Get() == nullptr)
			CreatePlugin(RootName);

		ModifiablePath = OutputPath;
		ModifiablePath.Split("/", &ModifiablePath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		ModifiablePath = ModifiablePath + "/";
	}

	const FString PathWithGame = ModifiablePath + Name;
	UPackage* Package = CreatePackage(nullptr, *PathWithGame);
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
bool FAssetUtilities::ConstructAsset(const FString& Path, const FString& Type, T*& OutObject, bool& bSuccess) {
	// Supported Assets
	if (Type == "Texture2D" ||
		Type == "TextureCube" ||
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
		Type == "MaterialFunction"
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

			FString RootName; {
				Path.Split("/", nullptr, &RootName, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				RootName.Split("/", &RootName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			}

			// Missing Plugin: Create it
			if (RootName != "Game" && RootName != "Engine" && IPluginManager::Get().FindPlugin(RootName).Get() == nullptr)
				CreatePlugin(RootName);

			bSuccess = Construct_TypeTexture(Path, Texture);
			if (bSuccess) OutObject = Cast<T>(Texture);

			return true;
		}
		else {
			const TSharedPtr<FJsonObject> Response = API_RequestExports(Path);
			if (Response.Get() == nullptr || Path.IsEmpty()) return true;

			TSharedPtr<FJsonObject> JsonObject = Response->GetArrayField("jsonOutput")[0]->AsObject();
			FString PackagePath;
			FString AssetName;
			Path.Split(".", &PackagePath, &AssetName);

			if (JsonObject) {
				UPackage* OutermostPkg;
				UPackage* Package = CreatePackage(nullptr, *PackagePath);
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

bool FAssetUtilities::Construct_TypeTexture(const FString& Path, UTexture*& OutTexture) {
	if (Path.IsEmpty())
		return false;

	TSharedPtr<FJsonObject> JsonObject = API_RequestExports(Path);
	if (JsonObject.Get() == nullptr)
		return false;

	TArray<TSharedPtr<FJsonValue>> Response = JsonObject->GetArrayField("jsonOutput");
	if (Response.Num() == 0)
		return false;

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	TSharedPtr<FJsonObject> JsonExport = Response[0]->AsObject();
	FString Type = JsonExport->GetStringField("Type");
	UTexture* Texture = nullptr;

	// --------------- Download Texture Data ------------
	FHttpModule* HttpModule = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> HttpRequest = HttpModule->CreateRequest();

	HttpRequest->SetURL(Settings->Url + "/api/v1/export?path=" + Path);
	HttpRequest->SetHeader("content-type", "application/octet-stream");
	HttpRequest->SetVerb(TEXT("GET"));

	const TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> HttpResponse = FRemoteUtilities::ExecuteRequestSync(HttpRequest);
	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
		return false;

	TArray<uint8> Data = HttpResponse->GetContent();
	if (Data.Num() == 0)
		return false;
	// --------------- Download Texture Data ------------

	FString PackagePath; FString AssetName; {
		Path.Split(".", &PackagePath, &AssetName);
	}

	UPackage* Package = CreatePackage(nullptr, *PackagePath);
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
		const FString PackageName = Package->GetName();
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		UPackage::SavePackage(Package, nullptr, RF_Standalone, *PackageFileName, GWarn, nullptr, false, true, SAVE_NoError);
	}

	OutTexture = Texture;

	return true;
}

void FAssetUtilities::CreatePlugin(FString PluginName) {
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

	const TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> NewResponse = FRemoteUtilities::ExecuteRequestSync(NewRequest);
	if (!NewResponse.IsValid()) return TSharedPtr<FJsonObject>();

	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(NewResponse->GetContentAsString());
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
		return JsonObject;

	return TSharedPtr<FJsonObject>();
}