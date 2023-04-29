#include "JsonAsAssetSettings.h"

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
