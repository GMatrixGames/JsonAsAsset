#include "JsonAsAssetSettings.h"

#define LOCTEXT_NAMESPACE "JsonAsAsset"

UJsonAsAssetSettings::UJsonAsAssetSettings() {
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("JsonAsAsset");

	bJumpToAsset = true;
	ExportFolder = FString("Output/Exports/");
}

#if WITH_EDITOR
FText UJsonAsAssetSettings::GetSectionText() const {
	return LOCTEXT("SettingsDisplayName", "JsonAsAsset");
}
#endif

#undef LOCTEXT_NAMESPACE
