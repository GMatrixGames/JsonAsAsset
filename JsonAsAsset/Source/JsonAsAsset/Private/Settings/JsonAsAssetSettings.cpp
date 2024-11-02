// Copyright JAA Contributors 2024-2025

#include "JsonAsAssetSettings.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Utilities/RemoteUtilities.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"

#define LOCTEXT_NAMESPACE "JsonAsAsset"

UJsonAsAssetSettings::UJsonAsAssetSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("JsonAsAsset");
}

#if WITH_EDITOR

FText UJsonAsAssetSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "JsonAsAsset");
}

TArray<FString> UJsonAsAssetSettings::GetParseVersions()
{
	TArray<FString> EnumNames;

	for (int Version = GAME_UE4_0; Version <= GAME_UE5_LATEST; ++Version)
	{
		EnumNames.Add(StaticEnum<EParseVersion>()->GetNameStringByIndex(Version));
	}

	return EnumNames;
}

#endif
#undef LOCTEXT_NAMESPACE