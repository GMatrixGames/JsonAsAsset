// Copyright Epic Games, Inc. All Rights Reserved.

#include "RemoteAssetDownloader.h"

#include "HttpModule.h"
#include "PackageHelperFunctions.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/TextureFactory.h"
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

	TSharedPtr<IHttpResponse> HttpResponse = FRemoteUtilities::ExecuteRequestSync(HttpRequest);

	if (HttpResponse.IsValid()) {
		FString PackagePath;
		FString AssetName;
		Path.Split(".", &PackagePath, &AssetName);
		UPackage* Package = FAssetUtilities::CreateAssetPackage(PackagePath);

		UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
		TextureFactory->AddToRoot();
		TextureFactory->SuppressImportOverwriteDialog();

		TArray<uint8> Data = HttpResponse->GetContent();
		const uint8* TextureData = Data.GetData();
		OutTexture = Cast<UTexture2D>(TextureFactory->FactoryCreateBinary(UTexture2D::StaticClass(), Package, FName(AssetName), RF_Standalone | RF_Public, nullptr,
		                                                                  *FPaths::GetExtension(AssetName + ".png").ToLower(), TextureData, TextureData + Data.Num(), GWarn));

		if (OutTexture != nullptr) {
			const TSharedRef<IHttpRequest> NewRequest = HttpModule->CreateRequest();
			NewRequest->SetURL("https://fortnitecentral.genxgames.gg/api/v1/export?raw=true&path=" + Path);
			NewRequest->SetVerb(TEXT("GET"));

			TSharedPtr<IHttpResponse> NewResponse = FRemoteUtilities::ExecuteRequestSync(NewRequest);

			if (HttpResponse.IsValid()) {
				const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(NewResponse->GetContentAsString());
				TSharedPtr<FJsonObject> JsonObject;
				if (FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
					const TSharedPtr<FJsonObject> TextureJson = JsonObject->GetArrayField("jsonOutput")[0]->AsObject();

					FString AddressX;
					if (TextureJson->GetObjectField("Properties")->TryGetStringField("AddressX", AddressX)) {
						OutTexture->AddressX = static_cast<TextureAddress>(FAssetUtilities::GetEnumOfType("/Script/Engine.TextureAddress")->GetValueByNameString(AddressX));
					}

					FString AddressY;
					if (TextureJson->GetObjectField("Properties")->TryGetStringField("AddressY", AddressY)) {
						OutTexture->AddressY = static_cast<TextureAddress>(FAssetUtilities::GetEnumOfType("/Script/Engine.TextureAddress")->GetValueByNameString(AddressY));
					}

					FString LightingGuid;
					if (TextureJson->GetObjectField("Properties")->TryGetStringField("LightingGuid", LightingGuid)) OutTexture->SetLightingGuid(FGuid(LightingGuid));

					FString LODGroup;
					if (TextureJson->GetObjectField("Properties")->TryGetStringField("LODGroup", LODGroup)) {
						OutTexture->LODGroup = static_cast<TextureGroup>(FAssetUtilities::GetEnumOfType("/Script/Engine.TextureGroup")->GetValueByNameString(LODGroup));
					}

					FTexturePlatformData* PlatformData = OutTexture->GetPlatformData();

					int SizeX;
					if (TextureJson->TryGetNumberField("SizeX", SizeX)) PlatformData->SizeX = SizeX;
					int SizeY;
					if (TextureJson->TryGetNumberField("SizeY", SizeY)) PlatformData->SizeY = SizeY;
					uint32 PackedData;
					if (TextureJson->TryGetNumberField("PackedData", PackedData)) PlatformData->PackedData = PackedData;
					FString PixelFormat;
					if (TextureJson->TryGetStringField("PixelFormat", PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(OutTexture->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

					FAssetRegistryModule::AssetCreated(OutTexture);
					if (!OutTexture->MarkPackageDirty()) return false;
					Package->SetDirtyFlag(true);
					OutTexture->PostEditChange();
					OutTexture->AddToRoot();
					// SavePackageHelper(OutTexture->GetPackage(), *OutTexture->GetName());
				}
			}
		}
	}

	return true;
}
