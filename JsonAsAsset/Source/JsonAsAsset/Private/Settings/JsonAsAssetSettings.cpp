#include "JsonAsAssetSettings.h"

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
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Reference to settings
	TWeakObjectPtr<UJsonAsAssetSettings> Settings = Cast<UJsonAsAssetSettings>(ObjectsBeingCustomized[0].Get());

	DetailBuilder.EditCategory("Asset", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch Encryption", FText::GetEmpty(), ECategoryPriority::Important);
	IDetailCategoryBuilder& EncryptionCategory = DetailBuilder.EditCategory("Local Fetch Encryption", FText::GetEmpty(), ECategoryPriority::Important);
		
	DetailBuilder.EditCategory("Local Fetch Director", FText::GetEmpty(), ECategoryPriority::Important);

	// keep for future use
	//EncryptionCategory.AddCustomRow(LOCTEXT("UpgradeInfo", "Upgrade Info"), false)
	//	.WholeRowWidget
	//	[
	//		SNew(SBorder)
	//		.Padding(1)
	//		[
	//			SNew(SHorizontalBox)
	//			+ SHorizontalBox::Slot()
	//			.Padding(FMargin(10, 10, 10, 10))
	//			.FillWidth(1.0f)
	//			[
	//				SNew(SRichTextBlock)
	//				.Text(LOCTEXT("UpgradeInfoMessage", "<RichTextBlock.TextHighlight>NOTE:</> Please fill these out correct so the Local Fetch API can fully start without any problems."))
	//				.TextStyle(FAppStyle::Get(), "MessageLog")
	//				.DecoratorStyleSet(&FAppStyle::Get())
	//				.AutoWrapText(true)
	//			]
	//		]
	//	];

	EncryptionCategory.AddCustomRow(LOCTEXT("EncryptionKeyGenerator", "EncryptionKeyGenerator"))
		.ValueContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("GenerateEncryptionKey", "[FORTNITE] Generate Encryption"))
			.ToolTipText(LOCTEXT("GenerateEncryptionKey_Tooltip", "Generates encryption keys from a Fortnite API."))
			.OnClicked_Lambda([this, Settings]() {
				UJsonAsAssetSettings* PluginSettings = GetMutableDefault<UJsonAsAssetSettings>();
				FHttpModule* HttpModule = &FHttpModule::Get();

				const TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
				Request->SetURL("https://fortnitecentral.genxgames.gg/api/v1/aes");
				Request->SetVerb(TEXT("GET"));

				const TSharedPtr<IHttpResponse> Response = FRemoteUtilities::ExecuteRequestSync(Request);
				if (!Response.IsValid()) return FReply::Handled();

				PluginSettings->DynamicKeys.Empty();

				const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
					PluginSettings->ArchiveKey = JsonObject->GetStringField("mainKey");

					for (const TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("dynamicKeys")) {
						const TSharedPtr<FJsonObject> Object = Value->AsObject();

						FString GUID = Object->GetStringField("guid");
						FString Key = Object->GetStringField("key");

						PluginSettings->DynamicKeys.Add(FParseKey(GUID, Key));
					}
				}

				PluginSettings->SaveConfig();

				return FReply::Handled();
			}).IsEnabled_Lambda([this, Settings]() {
				return Settings->bEnableLocalFetch;
			})
		];
}
#undef LOCTEXT_NAMESPACE