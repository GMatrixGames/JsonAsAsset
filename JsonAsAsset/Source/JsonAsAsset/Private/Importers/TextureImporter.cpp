// Copyright JAA Contributors 2023-2024

#include "Importers/TextureImporter.h"

#include "detex.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureCube.h"
#include "Engine/VolumeTexture.h"
#include "Factories/TextureRenderTargetFactoryNew.h"
#include "nvimage/DirectDrawSurface.h"
#include "nvimage/Image.h"
#include "Utilities/MathUtilities.h"
#include "Utilities/TextureDecode/TextureNVTT.h"

bool UTextureImporter::ImportTexture2D(UTexture*& OutTexture2D, TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	const TSharedPtr<FJsonObject> SubObjectProperties = Properties->GetObjectField("Properties");

	// NEW: .bin support
	UTexture2D* Texture2D = NewObject<UTexture2D>(OutermostPkg, UTexture2D::StaticClass(), *FileName, RF_Standalone | RF_Public);
	Texture2D->SetPlatformData(new FTexturePlatformData());

	ImportTexture2D_Data(Texture2D, SubObjectProperties);
	FTexturePlatformData* PlatformData = Texture2D->GetPlatformData();

	const int SizeX = Properties->GetNumberField("SizeX");
	const int SizeY = Properties->GetNumberField("SizeY");
	constexpr int SizeZ = 1; // Tex2D doesn't have depth

	if (FString PixelFormat; Properties->TryGetStringField("PixelFormat", PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(Texture2D->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	int Size = SizeX * SizeY * (PlatformData->PixelFormat == PF_BC6H ? 16 : 4);
	if (PlatformData->PixelFormat == PF_B8G8R8A8 || PlatformData->PixelFormat == PF_FloatRGBA || PlatformData->PixelFormat == PF_G16) Size = Data.Num();
	uint8* DecompressedData = static_cast<uint8*>(FMemory::Malloc(Size));

	GetDecompressedTextureData(Data.GetData(), DecompressedData, SizeX, SizeY, SizeZ, Size, PlatformData->PixelFormat);

	ETextureSourceFormat Format = TSF_BGRA8;
	if (Texture2D->CompressionSettings == TC_HDR) Format = TSF_RGBA16F;
	if (PlatformData->PixelFormat == PF_G16) Format = TSF_G16;
	Texture2D->Source.Init(SizeX, SizeY, 1, 1, Format);
	uint8_t* Dest = Texture2D->Source.LockMip(0);
	FMemory::Memcpy(Dest, DecompressedData, Size);
	Texture2D->Source.UnlockMip(0);

	Texture2D->UpdateResource();

	if (Texture2D) {
		OutTexture2D = Texture2D;
		return true;
	}

	return false;
}

bool UTextureImporter::ImportTextureCube(UTexture*& OutTextureCube, TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	UTextureCube* TextureCube = NewObject<UTextureCube>(Package, UTextureCube::StaticClass(), *FileName, RF_Public | RF_Standalone);

	TextureCube->SetPlatformData(new FTexturePlatformData());

	ImportTexture_Data(TextureCube, Properties);
	FTexturePlatformData* PlatformData = TextureCube->GetPlatformData();

	const int SizeX = Properties->GetNumberField("SizeX");
	const int SizeY = Properties->GetNumberField("SizeY") / 6;

	if (FString PixelFormat; Properties->TryGetStringField("PixelFormat", PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(TextureCube->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	int Size = SizeX * SizeY * (PlatformData->PixelFormat == PF_BC6H ? 16 : 4);
	if (PlatformData->PixelFormat == PF_FloatRGBA) Size = Data.Num();
	uint8* DecompressedData = static_cast<uint8*>(FMemory::Malloc(Size));

	ETextureSourceFormat Format = TSF_BGRA8;
	if (TextureCube->CompressionSettings == TC_HDR) Format = TSF_RGBA16F;
	TextureCube->Source.Init(SizeX, SizeY, 1, 1, Format);
	uint8_t* Dest = TextureCube->Source.LockMip(0);
	FMemory::Memcpy(Dest, DecompressedData, Size);
	TextureCube->Source.UnlockMip(0);

	TextureCube->PostEditChange();

	if (TextureCube) {
		OutTextureCube = TextureCube;
		return true;
	}

	return false;
}

bool UTextureImporter::ImportVolumeTexture(UTexture*& OutVolumeTexture, TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	UVolumeTexture* VolumeTexture = NewObject<UVolumeTexture>(Package, UVolumeTexture::StaticClass(), *FileName, RF_Public | RF_Standalone);

	VolumeTexture->SetPlatformData(new FTexturePlatformData());
	if (FString PixelFormat; Properties->TryGetStringField("PixelFormat", PixelFormat))
		VolumeTexture->GetPlatformData()->PixelFormat = static_cast<EPixelFormat>(VolumeTexture->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	ImportTexture_Data(VolumeTexture, Properties);

	const int SizeX = Properties->GetNumberField("SizeX");
	const int SizeY = Properties->GetNumberField("SizeY");
	// const int SizeZ = Properties->GetNumberField("SizeZ"); // Need to add the property
	const int SizeZ = 1;
	int Size = SizeX * SizeY * SizeZ;

	// Decompression
	uint8* DecompressedData = static_cast<uint8*>(FMemory::Malloc(Size));
	GetDecompressedTextureData(Data.GetData(), DecompressedData, SizeX, SizeY, SizeZ, Size, VolumeTexture->GetPlatformData()->PixelFormat);

	VolumeTexture->Source.Init(SizeX, SizeY, SizeZ, 1, TSF_BGRA8);

	uint8_t* Dest = VolumeTexture->Source.LockMip(0);
	FMemory::Memcpy(Dest, DecompressedData, Size);
	VolumeTexture->Source.UnlockMip(0);
	VolumeTexture->UpdateResource();

	if (VolumeTexture) {
		OutVolumeTexture = VolumeTexture;
		return true;
	}

	return false;
}

bool UTextureImporter::ImportRenderTarget2D(UTexture*& OutRenderTarget2D, const TSharedPtr<FJsonObject>& Properties) const {
	UTextureRenderTargetFactoryNew* TextureFactory = NewObject<UTextureRenderTargetFactoryNew>();
	TextureFactory->AddToRoot();
	UTextureRenderTarget2D* RenderTarget2D = Cast<UTextureRenderTarget2D>(TextureFactory->FactoryCreateNew(UTextureRenderTarget2D::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

	ImportTexture_Data(RenderTarget2D, Properties);

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
		if (FString MipsSamplerFilter; Properties->TryGetStringField("MipsSamplerFilter", MipsSamplerFilter))
			RenderTarget2D->MipsSamplerFilter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(MipsSamplerFilter));
	}

	const TSharedPtr<FJsonObject>* ClearColor;
	if (Properties->TryGetObjectField("ClearColor", ClearColor)) RenderTarget2D->ClearColor = FMathUtilities::ObjectToLinearColor(ClearColor->Get());

	if (RenderTarget2D) {
		OutRenderTarget2D = RenderTarget2D;
		return true;
	}

	return false;
}

// Handle UTexture2D
bool UTextureImporter::ImportTexture2D_Data(UTexture2D* InTexture2D, const TSharedPtr<FJsonObject>& Properties) const {
	if (InTexture2D == nullptr) return false;

	ImportTexture_Data(InTexture2D, Properties);

	if (FString AddressX; Properties->TryGetStringField("AddressX", AddressX)) InTexture2D->AddressX = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressX));
	if (FString AddressY; Properties->TryGetStringField("AddressY", AddressY)) InTexture2D->AddressY = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressY));
	if (bool bHasBeenPaintedInEditor; Properties->TryGetBoolField("bHasBeenPaintedInEditor", bHasBeenPaintedInEditor)) InTexture2D->bHasBeenPaintedInEditor = bHasBeenPaintedInEditor;

	// --------- Platform Data --------- //
	FTexturePlatformData* PlatformData = InTexture2D->GetPlatformData();

	if (int SizeX; Properties->TryGetNumberField("SizeX", SizeX)) PlatformData->SizeX = SizeX;
	if (int SizeY; Properties->TryGetNumberField("SizeY", SizeY)) PlatformData->SizeY = SizeY;
	if (uint32 PackedData; Properties->TryGetNumberField("PackedData", PackedData)) PlatformData->PackedData = PackedData;
	if (FString PixelFormat; Properties->TryGetStringField("PixelFormat", PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(InTexture2D->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	if (int FirstResourceMemMip; Properties->TryGetNumberField("FirstResourceMemMip", FirstResourceMemMip)) InTexture2D->FirstResourceMemMip = FirstResourceMemMip;
	if (int LevelIndex; Properties->TryGetNumberField("LevelIndex", LevelIndex)) InTexture2D->LevelIndex = LevelIndex;

	return false;
}

// Handle UTexture
bool UTextureImporter::ImportTexture_Data(UTexture* InTexture, const TSharedPtr<FJsonObject>& Properties) const {
	if (InTexture == nullptr) return false;

	// TODO: Replace with serializer
	if (float AdjustBrightness; Properties->TryGetNumberField("AdjustBrightness", AdjustBrightness))
		InTexture->AdjustBrightness = AdjustBrightness;
	if (float AdjustBrightnessCurve; Properties->TryGetNumberField("AdjustBrightnessCurve", AdjustBrightnessCurve))
		InTexture->AdjustBrightnessCurve = AdjustBrightnessCurve;
	if (float AdjustHue; Properties->TryGetNumberField("AdjustHue", AdjustHue))
		InTexture->AdjustHue = AdjustHue;
	if (float AdjustMaxAlpha; Properties->TryGetNumberField("AdjustMaxAlpha", AdjustMaxAlpha))
		InTexture->AdjustMaxAlpha = AdjustMaxAlpha;
	if (float AdjustMinAlpha; Properties->TryGetNumberField("AdjustMinAlpha", AdjustMinAlpha))
		InTexture->AdjustMinAlpha = AdjustMinAlpha;
	if (float AdjustRGBCurve; Properties->TryGetNumberField("AdjustRGBCurve", AdjustRGBCurve))
		InTexture->AdjustRGBCurve = AdjustRGBCurve;
	if (float AdjustSaturation; Properties->TryGetNumberField("AdjustSaturation", AdjustSaturation))
		InTexture->AdjustSaturation = AdjustSaturation;
	if (float AdjustVibrance; Properties->TryGetNumberField("AdjustVibrance", AdjustVibrance))
		InTexture->AdjustVibrance = AdjustVibrance;

	if (const TSharedPtr<FJsonObject>* AlphaCoverageThresholds; Properties->TryGetObjectField("AlphaCoverageThresholds", AlphaCoverageThresholds))
		InTexture->AlphaCoverageThresholds = FMathUtilities::ObjectToVector(AlphaCoverageThresholds->Get());

	if (bool bChromaKeyTexture; Properties->TryGetBoolField("bChromaKeyTexture", bChromaKeyTexture))
		InTexture->bChromaKeyTexture = bChromaKeyTexture;
	if (bool bFlipGreenChannel; Properties->TryGetBoolField("bFlipGreenChannel", bFlipGreenChannel))
		InTexture->bFlipGreenChannel = bFlipGreenChannel;
	if (bool bNoTiling; Properties->TryGetBoolField("bNoTiling", bNoTiling))
		InTexture->bNoTiling = bNoTiling;
	if (bool bPreserveBorder; Properties->TryGetBoolField("bPreserveBorder", bPreserveBorder))
		InTexture->bPreserveBorder = bPreserveBorder;
	if (bool bUseLegacyGamma; Properties->TryGetBoolField("bUseLegacyGamma", bUseLegacyGamma))
		InTexture->bUseLegacyGamma = bUseLegacyGamma;

	if (const TSharedPtr<FJsonObject>* ChromaKeyColor; Properties->TryGetObjectField("ChromaKeyColor", ChromaKeyColor))
		InTexture->ChromaKeyColor = FMathUtilities::ObjectToColor(ChromaKeyColor->Get());
	if (float ChromaKeyThreshold; Properties->TryGetNumberField("ChromaKeyThreshold", ChromaKeyThreshold))
		InTexture->ChromaKeyThreshold = ChromaKeyThreshold;

	if (float CompositePower; Properties->TryGetNumberField("CompositePower", CompositePower))
		InTexture->CompositePower = CompositePower;
	if (FString CompositeTextureMode; Properties->TryGetStringField("CompositeTextureMode", CompositeTextureMode))
		InTexture->CompositeTextureMode = static_cast<ECompositeTextureMode>(StaticEnum<ECompositeTextureMode>()->GetValueByNameString(CompositeTextureMode));

	if (bool CompressionNoAlpha; Properties->TryGetBoolField("CompressionNoAlpha", CompressionNoAlpha))
		InTexture->CompressionNoAlpha = CompressionNoAlpha;
	if (bool CompressionNone; Properties->TryGetBoolField("CompressionNone", CompressionNone))
		InTexture->CompressionNone = CompressionNone;
	if (FString CompressionQuality; Properties->TryGetStringField("CompressionQuality", CompressionQuality))
		InTexture->CompressionQuality = static_cast<ETextureCompressionQuality>(StaticEnum<ETextureCompressionQuality>()->GetValueByNameString(CompressionQuality));
	if (FString CompressionSettings; Properties->TryGetStringField("CompressionSettings", CompressionSettings))
		InTexture->CompressionSettings = static_cast<TextureCompressionSettings>(StaticEnum<TextureCompressionSettings>()->GetValueByNameString(CompressionSettings));
	if (bool CompressionYCoCg; Properties->TryGetBoolField("CompressionYCoCg", CompressionYCoCg))
		InTexture->CompressionYCoCg = CompressionYCoCg;
	if (bool DeferCompression; Properties->TryGetBoolField("DeferCompression", DeferCompression))
		InTexture->DeferCompression = DeferCompression;
	if (FString Filter; Properties->TryGetStringField("Filter", Filter))
		InTexture->Filter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(Filter));

	// TODO: Add LayerFormatSettings

	if (FString LODGroup; Properties->TryGetStringField("LODGroup", LODGroup))
		InTexture->LODGroup = static_cast<TextureGroup>(StaticEnum<TextureGroup>()->GetValueByNameString(LODGroup));
	if (FString LossyCompressionAmount; Properties->TryGetStringField("LossyCompressionAmount", LossyCompressionAmount))
		InTexture->LossyCompressionAmount = static_cast<ETextureLossyCompressionAmount>(StaticEnum<ETextureLossyCompressionAmount>()->GetValueByNameString(LossyCompressionAmount));

	if (int MaxTextureSize; Properties->TryGetNumberField("MaxTextureSize", MaxTextureSize))
		InTexture->MaxTextureSize = MaxTextureSize;
	if (FString MipGenSettings; Properties->TryGetStringField("MipGenSettings", MipGenSettings))
		InTexture->MipGenSettings = static_cast<TextureMipGenSettings>(StaticEnum<TextureMipGenSettings>()->GetValueByNameString(MipGenSettings));
	if (FString MipLoadOptions; Properties->TryGetStringField("MipLoadOptions", MipLoadOptions))
		InTexture->MipLoadOptions = static_cast<ETextureMipLoadOptions>(StaticEnum<ETextureMipLoadOptions>()->GetValueByNameString(MipLoadOptions));

	if (const TSharedPtr<FJsonObject>* PaddingColor; Properties->TryGetObjectField("PaddingColor", PaddingColor)) InTexture->PaddingColor = FMathUtilities::ObjectToColor(PaddingColor->Get());
	if (FString PowerOfTwoMode; Properties->TryGetStringField("PowerOfTwoMode", PowerOfTwoMode))
		InTexture->PowerOfTwoMode = static_cast<ETexturePowerOfTwoSetting::Type>(StaticEnum<ETexturePowerOfTwoSetting::Type>()->GetValueByNameString(PowerOfTwoMode));

	if (bool SRGB; Properties->TryGetBoolField("SRGB", SRGB))
		InTexture->SRGB = SRGB;
	if (bool VirtualTextureStreaming; Properties->TryGetBoolField("VirtualTextureStreaming", VirtualTextureStreaming))
		InTexture->VirtualTextureStreaming = VirtualTextureStreaming;

	if (FString LightingGuid; Properties->TryGetStringField("LightingGuid", LightingGuid))
		InTexture->SetLightingGuid(FGuid(LightingGuid));

	return false;
}

void UTextureImporter::GetDecompressedTextureData(uint8* Data, uint8*& OutData, const int SizeX, const int SizeY, const int SizeZ, const int TotalSize, const EPixelFormat Format) const {
	// NOTE: Not all formats are supported, feel free to add
	//       if needed. Formats may need other dependencies.
	switch (Format) {
	case PF_BC7: {
		detexTexture Texture;
		Texture.data = Data;
		Texture.format = DETEX_TEXTURE_FORMAT_BPTC;
		Texture.width = SizeX;
		Texture.height = SizeY;
		Texture.width_in_blocks = SizeX / 4;
		Texture.height_in_blocks = SizeY / 4;

		detexDecompressTextureLinear(&Texture, OutData, DETEX_PIXEL_FORMAT_BGRA8);
	}
	break;

	case PF_BC6H: {
		detexTexture Texture;
		Texture.data = Data;
		Texture.format = DETEX_TEXTURE_FORMAT_BPTC_FLOAT;
		Texture.width = SizeX;
		Texture.height = SizeY;
		Texture.width_in_blocks = SizeX / 4;
		Texture.height_in_blocks = SizeY / 4;

		detexDecompressTextureLinear(&Texture, OutData, DETEX_PIXEL_FORMAT_BGRA8);
	}
	break;

	case PF_DXT5: {
		detexTexture Texture;
		{
			Texture.data = Data;
			Texture.format = DETEX_TEXTURE_FORMAT_BC3;
			Texture.width = SizeX;
			Texture.height = SizeY;
			Texture.width_in_blocks = SizeX / 4;
			Texture.height_in_blocks = SizeY / 4;
		}

		detexDecompressTextureLinear(&Texture, OutData, DETEX_PIXEL_FORMAT_BGRA8);
	}
	break;

	// Gray/Grey, not Green, typically actually uses a red format with replication of R to RGB
	case PF_G8: {
		const uint8* s = Data;
		uint8* d = OutData;

		for (int i = 0; i < SizeX * SizeY; i++) {
			const uint8 b = *s++;
			*d++ = b;
			*d++ = b;
			*d++ = b;
			*d++ = 255;
		}
	}
	break;

	// FloatRGBA: 16F
	// G16: Gray/Grey like G8
	case PF_B8G8R8A8:
	case PF_FloatRGBA:
	case PF_G16: {
		FMemory::Memcpy(OutData, Data, TotalSize);
	}
	break;

	default: {
		nv::DDSHeader Header;
		nv::Image Image;

		uint FourCC;
		switch (Format) {
		case PF_BC4:
			FourCC = FOURCC_ATI1;
			break;
		case PF_BC5:
			FourCC = FOURCC_ATI2;
			break;
		case PF_DXT1:
			FourCC = FOURCC_DXT1;
			break;
		case PF_DXT3:
			FourCC = FOURCC_DXT3;
			break;
		default: FourCC = 0;
		}

		Header.setFourCC(FourCC);
		Header.setWidth(SizeX);
		Header.setHeight(SizeY);
		Header.setDepth(SizeZ);
		Header.setNormalFlag(Format == PF_BC5);
		DecodeDDS(Data, SizeX, SizeY, SizeZ, Header, Image);

		FMemory::Memcpy(OutData, Image.pixels(), TotalSize);
	}
	break;
	}
}
