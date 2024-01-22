// Copyright JAA Contributors 2023-2024

#include "JsonAsAssetStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FJsonAsAssetStyle::StyleInstance = nullptr;

void FJsonAsAssetStyle::Initialize() {
	if (!StyleInstance.IsValid()) {
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FJsonAsAssetStyle::Shutdown() {
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FJsonAsAssetStyle::GetStyleSetName() {
	static FName StyleSetName(TEXT("JsonAsAssetStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BORDER_BRUSH(RelativePath, ...) FSlateBorderBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define TTF_FONT(RelativePath, ...) FSlateFontInfo(Style->RootToContentDir(RelativePath, TEXT(".ttf")), __VA_ARGS__)
#define OTF_FONT(RelativePath, ...) FSlateFontInfo(Style->RootToContentDir(RelativePath, TEXT(".otf")), __VA_ARGS__)

const FVector2D Icon16x16(16, 16);
const FVector2D Icon20x20(20, 20);
const FVector2D Icon40x40(40, 40);
const FVector2D IconFullScreen(720, 300);

TSharedRef<FSlateStyleSet> FJsonAsAssetStyle::Create() {
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("JsonAsAssetStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("JsonAsAsset")->GetBaseDir() / TEXT("Resources"));

	Style->Set("JsonAsAsset.PluginAction", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("JsonAsAsset.FModelLogo", new IMAGE_BRUSH(TEXT("ButtonIcon_FModel"), Icon40x40));
	Style->Set("JsonAsAsset.GithubLogo", new IMAGE_BRUSH(TEXT("ButtonIcon_Github"), Icon40x40));
	Style->Set("JsonAsAsset.AboutScreen", new IMAGE_BRUSH(TEXT("Icon_FullScreen"), IconFullScreen));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FJsonAsAssetStyle::ReloadTextures() {
	if (FSlateApplication::IsInitialized()) {
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FJsonAsAssetStyle::Get() {
	return *StyleInstance;
}
