// Copyright Epic Games, Inc. All Rights Reserved.

#include "RemoteAssetDownloader.h"

#include "HttpModule.h"
#include "Importers/TextureImporters.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/RemoteUtilities.h"

bool FRemoteAssetDownloader::MakeTexture(const FString& Path, UTexture2D*& OutTexture) {
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
		UPackage* Package = FAssetUtilities::CreateAssetPackage(Path);
		const UTextureImporters* Importer = new UTextureImporters(AssetName, JsonObject, Package, nullptr);

		UTexture* Texture;
		Importer->ImportTexture2D(Texture, Data, JsonObject->GetArrayField("jsonOutput")[0]->AsObject());

		OutTexture = Cast<UTexture2D>(Texture);
	}

	return true;
}
