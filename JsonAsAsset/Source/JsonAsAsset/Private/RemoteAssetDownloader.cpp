// Copyright Epic Games, Inc. All Rights Reserved.

#include "RemoteAssetDownloader.h"

#include "HttpModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Importers/TextureImporters.h"
#include "Importers/MaterialParameterCollectionImporter.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/RemoteUtilities.h"

template <typename T>
bool FRemoteAssetDownloader::DownloadAsset(const FString& Path, const FString& Type, TObjectPtr<T>& OutObject, bool& bSuccess)
{
	// Supported Assets
	if (Type ==
		"Texture2D" ||
		Type == "TextureRenderTarget2D" ||
		Type == "MaterialParameterCollection"
	) {
		//		Manually supported asset types
		// (ex: textures have to be handled differently)
		if (Type ==
			"Texture2D" ||
			Type == "TextureRenderTarget2D"
		) {
			UTexture* Texture;

			bSuccess = MakeTexture(Path, Texture);
			if (bSuccess) OutObject = Cast<T>(Texture);

			return true;
		} else {
			const TSharedPtr<FJsonObject> Response = RequestExports(Path);
			if (Response == nullptr) return true;

			TSharedPtr<FJsonObject> JsonObject = Response->GetArrayField("jsonOutput")[0]->AsObject();
			FString PackagePath;
			FString AssetName;
			Path.Split(".", &PackagePath, &AssetName);

			if (JsonObject) {
				UPackage* OutermostPkg;
				UPackage* Package = CreatePackage(*PackagePath);
				OutermostPkg = Package->GetOutermost();
				Package->FullyLoad();

				// Import asset by IImporter
				IImporter* Importer = new IImporter();
				bSuccess = Importer->HandleExports(Response->GetArrayField("jsonOutput"), PackagePath);

				// Define found object
				OutObject = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *Path));

				return true;
			}
		}
	}

	return false;
}

bool FRemoteAssetDownloader::MakeTexture(const FString& Path, UTexture*& OutTexture) {
	FHttpModule* HttpModule = &FHttpModule::Get();
	const TSharedRef<IHttpRequest> HttpRequest = HttpModule->CreateRequest();

	HttpRequest->SetURL("https://fortnitecentral.genxgames.gg/api/v1/export?path=" + Path);
	HttpRequest->SetHeader("content-type", "image/png");
	HttpRequest->SetVerb(TEXT("GET"));

	const TSharedPtr<IHttpResponse> HttpResponse = FRemoteUtilities::ExecuteRequestSync(HttpRequest);
	if (!HttpResponse.IsValid()) return false;

	const TArray<uint8> Data = HttpResponse->GetContent();

	FString PackagePath;
	FString AssetName;
	Path.Split(".", &PackagePath, &AssetName);

	const TSharedRef<IHttpRequest> NewRequest = HttpModule->CreateRequest();
	NewRequest->SetURL("https://fortnitecentral.genxgames.gg/api/v1/export?raw=true&path=" + Path);
	NewRequest->SetVerb(TEXT("GET"));

	const TSharedPtr<IHttpResponse> NewResponse = FRemoteUtilities::ExecuteRequestSync(NewRequest);
	if (!NewResponse.IsValid()) return false;

	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(NewResponse->GetContentAsString());
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
		UPackage* OutermostPkg;
		UPackage* Package = CreatePackage(*PackagePath);
		OutermostPkg = Package->GetOutermost();
		Package->FullyLoad();

		const UTextureImporters* Importer = new UTextureImporters(AssetName, Path, JsonObject, Package, OutermostPkg);
		TSharedPtr<FJsonObject> FinalJsonObject = JsonObject->GetArrayField("jsonOutput")[0]->AsObject();

		UTexture* Texture = nullptr;
		
		// Texture 2D
		if (FinalJsonObject->GetStringField("Type") == "Texture2D")
			Importer->ImportTexture2D(Texture, Data, FinalJsonObject->GetObjectField("Properties"));
		// Texture Render Target 2D
		if (FinalJsonObject->GetStringField("Type") == "TextureRenderTarget2D")
			Importer->ImportRenderTarget2D(Texture, FinalJsonObject->GetObjectField("Properties"));

		// If it still wasn't imported
		if (Texture == nullptr) return false;

		FAssetRegistryModule::AssetCreated(Texture);
		if (!Texture->MarkPackageDirty()) return false;
		Package->SetDirtyFlag(true);
		Texture->PostEditChange();
		Texture->AddToRoot();

		OutTexture = Texture;
	}

	return true;
}

bool FRemoteAssetDownloader::MakeMaterialParameterCollection(const FString& Path, UMaterialParameterCollection*& OutCollection) {
	const TSharedPtr<FJsonObject> Response = RequestExports(Path);
	if (Response == nullptr) return false;

	TSharedPtr<FJsonObject> JsonObject = Response->GetArrayField("jsonOutput")[0]->AsObject();
	FString PackagePath;
	FString AssetName;
	Path.Split(".", &PackagePath, &AssetName);

	if (JsonObject) {
		UPackage* OutermostPkg;
		UPackage* Package = CreatePackage(*PackagePath);
		OutermostPkg = Package->GetOutermost();
		Package->FullyLoad();

		UMaterialParameterCollectionImporter* MaterialCollectionImporter = new UMaterialParameterCollectionImporter(AssetName, Path, JsonObject, Package, OutermostPkg, Response->GetArrayField("jsonOutput"));
		MaterialCollectionImporter->ImportData();

		OutCollection = MaterialCollectionImporter->OutCollection;
	} else return false;

	return true;
}

const TSharedPtr<FJsonObject> FRemoteAssetDownloader::RequestExports(const FString& Path)
{
	FHttpModule* HttpModule = &FHttpModule::Get();
	const TSharedRef<IHttpRequest> HttpRequest = HttpModule->CreateRequest();

	FString PackagePath;
	FString AssetName;
	Path.Split(".", &PackagePath, &AssetName);

	const TSharedRef<IHttpRequest> NewRequest = HttpModule->CreateRequest();
	NewRequest->SetURL("https://fortnitecentral.genxgames.gg/api/v1/export?raw=true&path=" + Path);
	NewRequest->SetVerb(TEXT("GET"));

	const TSharedPtr<IHttpResponse> NewResponse = FRemoteUtilities::ExecuteRequestSync(NewRequest);
	if (!NewResponse.IsValid()) return TSharedPtr<FJsonObject>();

	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(NewResponse->GetContentAsString());
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
		return JsonObject;
	}

	return TSharedPtr<FJsonObject>();
}
