#include "JsonAsAssetSettings.h"

#define LOCTEXT_NAMESPACE "JsonAsAsset"

UJsonAsAssetSettings::UJsonAsAssetSettings() {
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("JsonAsAsset");

	bAutomateReferences = true;
}

#if WITH_EDITOR
FText UJsonAsAssetSettings::GetSectionText() const {
	return LOCTEXT("SettingsDisplayName", "JsonAsAsset");
}
#endif
#undef LOCTEXT_NAMESPACE
