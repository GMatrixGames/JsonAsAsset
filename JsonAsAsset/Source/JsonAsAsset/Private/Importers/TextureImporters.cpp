#include "Importers/TextureImporters.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Factories/TextureFactory.h"
#include "Factories/TextureRenderTargetFactoryNew.h"
#include "Utilities/MathUtilities.h"

bool UTextureImporters::ImportData() {
	try {
		const FString Type = JsonObject->GetStringField("Type");

		UTexture* OutTexture = nullptr;

		if (Type == "TextureRenderTarget2D") ImportRenderTarget2D(OutTexture, JsonObject->GetObjectField("Properties"));

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(OutTexture)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception))
		return false;
	}

	return true;
}

bool UTextureImporters::ImportTexture2D(UTexture*& OutTexture2D, const TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	TextureFactory->AddToRoot();
	TextureFactory->SuppressImportOverwriteDialog();

	const uint8* ImageData = Data.GetData();
	UTexture2D* Texture2D = Cast<UTexture2D>(TextureFactory->FactoryCreateBinary(UTexture2D::StaticClass(), Package, *FileName, RF_Standalone | RF_Public, nullptr,
	                                                                           *FPaths::GetExtension(FileName + ".png").ToLower(), ImageData, ImageData + Data.Num(), GWarn));
	if (Texture2D == nullptr)
		return false;

	FString AddressX;
	if (Properties->TryGetStringField("AddressX", AddressX)) Texture2D->AddressX = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressX));
	FString AddressY;
	if (Properties->TryGetStringField("AddressY", AddressY)) Texture2D->AddressY = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressY));
	FString LightingGuid;
	if (Properties->TryGetStringField("LightingGuid", LightingGuid)) Texture2D->SetLightingGuid(FGuid(LightingGuid));
	FString MipLoadOptions;
	if (Properties->TryGetStringField("MipLoadOptions", MipLoadOptions)) Texture2D->MipLoadOptions = static_cast<ETextureMipLoadOptions>(StaticEnum<ETextureMipLoadOptions>()->GetValueByNameString(MipLoadOptions));
	FString CompressionSettings;
	if (Properties->TryGetStringField("CompressionSettings", CompressionSettings)) Texture2D->CompressionSettings = static_cast<TextureCompressionSettings>(StaticEnum<TextureCompressionSettings>()->GetValueByNameString(CompressionSettings));
	FString Filter;
	if (Properties->TryGetStringField("Filter", Filter)) Texture2D->Filter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(Filter));
	FString LODGroup;
	if (Properties->TryGetStringField("LODGroup", LODGroup)) Texture2D->LODGroup = static_cast<TextureGroup>(StaticEnum<TextureGroup>()->GetValueByNameString(LODGroup));

	bool SRGB;
	if (Properties->TryGetBoolField("SRGB", SRGB)) Texture2D->SRGB = SRGB;
	bool NeverStream;
	if (Properties->TryGetBoolField("NeverStream", NeverStream)) Texture2D->NeverStream = NeverStream;
	bool bGlobalForceMipLevelsToBeResident;
	if (Properties->TryGetBoolField("bGlobalForceMipLevelsToBeResident", bGlobalForceMipLevelsToBeResident)) Texture2D->bGlobalForceMipLevelsToBeResident = bGlobalForceMipLevelsToBeResident;

	FTexturePlatformData* PlatformData = Texture2D->GetPlatformData();

	int SizeX;
	if (Properties->TryGetNumberField("SizeX", SizeX)) PlatformData->SizeX = SizeX;
	int SizeY;
	if (Properties->TryGetNumberField("SizeY", SizeY)) PlatformData->SizeY = SizeY;
	uint32 PackedData;
	if (Properties->TryGetNumberField("PackedData", PackedData)) PlatformData->PackedData = PackedData;
	FString PixelFormat;
	if (Properties->TryGetStringField("PixelFormat", PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(Texture2D->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	if (Texture2D) {
		OutTexture2D = Texture2D;
		return true;
	}

	return false;
}

bool UTextureImporters::ImportRenderTarget2D(UTexture*& OutRenderTarget2D, const TSharedPtr<FJsonObject>& Properties) const {
	UTextureRenderTargetFactoryNew* TextureFactory = NewObject<UTextureRenderTargetFactoryNew>();
	TextureFactory->AddToRoot();
	UTextureRenderTarget2D* RenderTarget2D = Cast<UTextureRenderTarget2D>(TextureFactory->FactoryCreateNew(UTextureRenderTarget2D::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

	int SizeX;
	if (Properties->TryGetNumberField("SizeX", SizeX)) RenderTarget2D->SizeX = SizeX;
	int SizeY;
	if (Properties->TryGetNumberField("SizeY", SizeY)) RenderTarget2D->SizeY = SizeY;
	
	FString AddressX;
	if (Properties->TryGetStringField("AddressX", AddressX)) RenderTarget2D->AddressX = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressX));
	FString AddressY;
	if (Properties->TryGetStringField("AddressY", AddressY)) RenderTarget2D->AddressY = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressY));
	FString RenderTargetFormat;
	if (Properties->TryGetStringField("RenderTargetFormat", RenderTargetFormat)) RenderTarget2D->RenderTargetFormat = static_cast<ETextureRenderTargetFormat>(StaticEnum<ETextureRenderTargetFormat>()->GetValueByNameString(RenderTargetFormat));

	bool bAutoGenerateMips;
	if (Properties->TryGetBoolField("bAutoGenerateMips", bAutoGenerateMips)) RenderTarget2D->bAutoGenerateMips = bAutoGenerateMips;

	if (bAutoGenerateMips) {
		FString MipsSamplerFilter;
		if (Properties->TryGetStringField("MipsSamplerFilter", MipsSamplerFilter)) RenderTarget2D->MipsSamplerFilter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(MipsSamplerFilter));
	}
	
	FString LightingGuid;
	if (Properties->TryGetStringField("LightingGuid", LightingGuid)) RenderTarget2D->SetLightingGuid(FGuid(LightingGuid));
	bool SRGB;
	if (Properties->TryGetBoolField("SRGB", SRGB)) RenderTarget2D->SRGB = SRGB;

	FString LODGroup;
	if (Properties->TryGetStringField("LODGroup", LODGroup)) RenderTarget2D->LODGroup = static_cast<TextureGroup>(StaticEnum<TextureGroup>()->GetValueByNameString(LODGroup));

	const TSharedPtr<FJsonObject>* ClearColor;
	if (Properties->TryGetObjectField("ClearColor", ClearColor)) RenderTarget2D->ClearColor = FMathUtilities::ObjectToLinearColor(ClearColor->Get());
	
	if (RenderTarget2D) {
		OutRenderTarget2D = RenderTarget2D;
		return true;
	}

	return false;
}
