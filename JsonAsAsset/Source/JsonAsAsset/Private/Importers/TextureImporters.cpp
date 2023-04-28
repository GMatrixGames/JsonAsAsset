#include "Importers/TextureImporters.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureCube.h"
#include "Factories/TextureFactory.h"
#include "Factories/TextureRenderTargetFactoryNew.h"
#include "Utilities/MathUtilities.h"
#include <IImageWrapper.h>

#include "Factories.h"
#include "IImageWrapperModule.h"

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

	ImportTexture2D_Data(Texture2D, Properties);

	if (Texture2D) {
		OutTexture2D = Texture2D;
		return true;
	}

	return false;
}

bool UTextureImporters::ImportTextureCube(UTexture*& OutTextureCube, const TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	// create the cube texture
	UTextureCube* TextureCube = NewObject<UTextureCube>(Package, UTextureCube::StaticClass(), *FileName, RF_Public | RF_Standalone);

	const uint8* ImageData = Data.GetData();

	ImportTexture_Data(TextureCube, Properties->GetObjectField("Properties"));

	float InSizeX = Properties->GetNumberField("SizeX");
	float InSizeY = Properties->GetNumberField("SizeY");

	EPixelFormat InFormat = static_cast<EPixelFormat>(StaticEnum<EPixelFormat>()->GetValueByNameString(Properties->GetStringField("PixelFormat")));

	TextureCube->SetPlatformData(new FTexturePlatformData());
	TextureCube->GetPlatformData()->SizeX = InSizeX;
	TextureCube->GetPlatformData()->SizeY = InSizeY;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	const int64 Length = *ImageData + Data.Num();

	TSharedPtr<IImageWrapper> HdrImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::HDR);
	if (HdrImageWrapper->SetCompressed(ImageData, Length)) {
		TArray64<uint8> UnCompressedData;
		if (HdrImageWrapper->GetRaw(ERGBFormat::BGRE, 8, UnCompressedData)) {
			if (TextureCube) {
				TextureCube->Source.Init(
					HdrImageWrapper->GetWidth(),
					HdrImageWrapper->GetHeight(),
					/*NumSlices=*/ 1,
					/*NumMips=*/ 1,
					TSF_BGRE8,
					UnCompressedData.GetData()
				);

				TextureCube->CompressionSettings = TC_HDR;
				TextureCube->SRGB = false;

				UE_LOG(LogEditorFactories, Display, TEXT("HDR Image imported as LongLat cube map."));
			}
		}
	}

	if (TextureCube) {
		OutTextureCube = TextureCube;
		return true;
	}

	return false;
}

bool UTextureImporters::ImportRenderTarget2D(UTexture*& OutRenderTarget2D, const TSharedPtr<FJsonObject>& Properties) const {
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
bool UTextureImporters::ImportTexture2D_Data(UTexture2D* InTexture2D, const TSharedPtr<FJsonObject>& Properties) const {
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
bool UTextureImporters::ImportTexture_Data(UTexture* InTexture, const TSharedPtr<FJsonObject>& Properties) const {
	if (InTexture == nullptr) return false;

	// Adjust parameters
	if (float AdjustBrightness; Properties->TryGetNumberField("AdjustBrightness", AdjustBrightness)) InTexture->AdjustBrightness = AdjustBrightness;
	if (float AdjustBrightnessCurve; Properties->TryGetNumberField("AdjustBrightnessCurve", AdjustBrightnessCurve)) InTexture->AdjustBrightnessCurve = AdjustBrightnessCurve;
	if (float AdjustHue; Properties->TryGetNumberField("AdjustHue", AdjustHue)) InTexture->AdjustHue = AdjustHue;
	if (float AdjustMaxAlpha; Properties->TryGetNumberField("AdjustMaxAlpha", AdjustMaxAlpha)) InTexture->AdjustMaxAlpha = AdjustMaxAlpha;
	if (float AdjustMinAlpha; Properties->TryGetNumberField("AdjustMinAlpha", AdjustMinAlpha)) InTexture->AdjustMinAlpha = AdjustMinAlpha;
	if (float AdjustRGBCurve; Properties->TryGetNumberField("AdjustRGBCurve", AdjustRGBCurve)) InTexture->AdjustRGBCurve = AdjustRGBCurve;
	if (float AdjustSaturation; Properties->TryGetNumberField("AdjustSaturation", AdjustSaturation)) InTexture->AdjustSaturation = AdjustSaturation;
	if (float AdjustVibrance; Properties->TryGetNumberField("AdjustVibrance", AdjustVibrance)) InTexture->AdjustVibrance = AdjustVibrance;

	if (const TSharedPtr<FJsonObject>* AlphaCoverageThresholds; Properties->TryGetObjectField("AlphaCoverageThresholds", AlphaCoverageThresholds))
		InTexture->AlphaCoverageThresholds = FMathUtilities::ObjectToVector(AlphaCoverageThresholds->Get());

	if (bool bChromaKeyTexture; Properties->TryGetBoolField("bChromaKeyTexture", bChromaKeyTexture)) InTexture->bChromaKeyTexture = bChromaKeyTexture;
	if (bool bFlipGreenChannel; Properties->TryGetBoolField("bFlipGreenChannel", bFlipGreenChannel)) InTexture->bFlipGreenChannel = bFlipGreenChannel;
	if (bool bNoTiling; Properties->TryGetBoolField("bNoTiling", bNoTiling)) InTexture->bNoTiling = bNoTiling;
	if (bool bPreserveBorder; Properties->TryGetBoolField("bPreserveBorder", bPreserveBorder)) InTexture->bPreserveBorder = bPreserveBorder;
	if (bool bUseLegacyGamma; Properties->TryGetBoolField("bUseLegacyGamma", bUseLegacyGamma)) InTexture->bUseLegacyGamma = bUseLegacyGamma;

	if (const TSharedPtr<FJsonObject>* ChromaKeyColor; Properties->TryGetObjectField("ChromaKeyColor", ChromaKeyColor)) InTexture->ChromaKeyColor = FMathUtilities::ObjectToColor(ChromaKeyColor->Get());
	if (float ChromaKeyThreshold; Properties->TryGetNumberField("ChromaKeyThreshold", ChromaKeyThreshold)) InTexture->ChromaKeyThreshold = ChromaKeyThreshold;

	if (float CompositePower; Properties->TryGetNumberField("CompositePower", CompositePower)) InTexture->CompositePower = CompositePower;
	// if (const TSharedPtr<FJsonObject>* CompositeTexture; Properties->TryGetObjectField("CompositeTexture", CompositeTexture));
	if (FString CompositeTextureMode; Properties->TryGetStringField("CompositeTextureMode", CompositeTextureMode)) InTexture->CompositeTextureMode = static_cast<ECompositeTextureMode>(StaticEnum<ECompositeTextureMode>()->GetValueByNameString(CompositeTextureMode));

	if (bool CompressionNoAlpha; Properties->TryGetBoolField("CompressionNoAlpha", CompressionNoAlpha)) InTexture->CompressionNoAlpha = CompressionNoAlpha;
	if (bool CompressionNone; Properties->TryGetBoolField("CompressionNone", CompressionNone)) InTexture->CompressionNone = CompressionNone;
	if (FString CompressionQuality; Properties->TryGetStringField("CompressionQuality", CompressionQuality)) InTexture->CompressionQuality = static_cast<ETextureCompressionQuality>(StaticEnum<ETextureCompressionQuality>()->GetValueByNameString(CompressionQuality));
	if (FString CompressionSettings; Properties->TryGetStringField("CompressionSettings", CompressionSettings)) InTexture->CompressionSettings = static_cast<TextureCompressionSettings>(StaticEnum<TextureCompressionSettings>()->GetValueByNameString(CompressionSettings));
	if (bool CompressionYCoCg; Properties->TryGetBoolField("CompressionYCoCg", CompressionYCoCg)) InTexture->CompressionYCoCg = CompressionYCoCg;
	if (bool DeferCompression; Properties->TryGetBoolField("DeferCompression", DeferCompression)) InTexture->DeferCompression = DeferCompression;
	if (FString Filter; Properties->TryGetStringField("Filter", Filter)) InTexture->Filter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(Filter));

	// TODO: Add LayerFormatSettings

	if (int LODBias; Properties->TryGetNumberField("LODBias", LODBias)) InTexture->LODBias = LODBias;
	if (FString LODGroup; Properties->TryGetStringField("LODGroup", LODGroup)) InTexture->LODGroup = static_cast<TextureGroup>(StaticEnum<TextureGroup>()->GetValueByNameString(LODGroup));
	if (FString LossyCompressionAmount; Properties->TryGetStringField("LossyCompressionAmount", LossyCompressionAmount)) InTexture->LossyCompressionAmount = static_cast<ETextureLossyCompressionAmount>(StaticEnum<ETextureLossyCompressionAmount>()->GetValueByNameString(LossyCompressionAmount));

	if (int MaxTextureSize; Properties->TryGetNumberField("MaxTextureSize", MaxTextureSize)) InTexture->MaxTextureSize = MaxTextureSize;
	if (FString MipGenSettings; Properties->TryGetStringField("MipGenSettings", MipGenSettings)) InTexture->MipGenSettings = static_cast<TextureMipGenSettings>(StaticEnum<TextureMipGenSettings>()->GetValueByNameString(MipGenSettings));
	if (FString MipLoadOptions; Properties->TryGetStringField("MipLoadOptions", MipLoadOptions)) InTexture->MipLoadOptions = static_cast<ETextureMipLoadOptions>(StaticEnum<ETextureMipLoadOptions>()->GetValueByNameString(MipLoadOptions));

	if (const TSharedPtr<FJsonObject>* PaddingColor; Properties->TryGetObjectField("PaddingColor", PaddingColor)) InTexture->PaddingColor = FMathUtilities::ObjectToColor(PaddingColor->Get());
	if (FString PowerOfTwoMode; Properties->TryGetStringField("PowerOfTwoMode", PowerOfTwoMode)) InTexture->PowerOfTwoMode = static_cast<ETexturePowerOfTwoSetting::Type>(StaticEnum<ETexturePowerOfTwoSetting::Type>()->GetValueByNameString(PowerOfTwoMode));

	if (bool SRGB; Properties->TryGetBoolField("SRGB", SRGB)) InTexture->SRGB = SRGB;
	if (bool VirtualTextureStreaming; Properties->TryGetBoolField("VirtualTextureStreaming", VirtualTextureStreaming)) InTexture->VirtualTextureStreaming = VirtualTextureStreaming;

	if (FString LightingGuid; Properties->TryGetStringField("LightingGuid", LightingGuid)) InTexture->SetLightingGuid(FGuid(LightingGuid));

	return false;
}
