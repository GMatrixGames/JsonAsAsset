#include "JsonAsAssetSettings.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Framework/Text/SlateHyperlinkRun.h"

#include "Utilities/AssetUtilities.h"
#include "Utilities/RemoteUtilities.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"

#define LOCTEXT_NAMESPACE "JsonAsAsset"

UJsonAsAssetSettings::UJsonAsAssetSettings() {
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("JsonAsAsset");
}

#if WITH_EDITOR
FText UJsonAsAssetSettings::GetSectionText() const {
	return LOCTEXT("SettingsDisplayName", "JsonAsAsset");
}

TArray<FString> UJsonAsAssetSettings::GetParseVersions() {
	TArray<FString> EnumNames;
	const UEnum* VersionEnum = StaticEnum<EParseVersion>();
	for (int Version = GAME_UE4_0; Version != GAME_UE5_LATEST; ++Version) {
		EnumNames.Add(StaticEnum<EParseVersion>()->GetNameStringByIndex(Version));
	}
	return EnumNames;
}
#endif
#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "JsonAsAssetSettingsDetails"
TSharedRef<IDetailCustomization> FJsonAsAssetSettingsDetails::MakeInstance() {
	return MakeShareable(new FJsonAsAssetSettingsDetails);
}

void FJsonAsAssetSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) {
}
#undef LOCTEXT_NAMESPACE