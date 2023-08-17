#include "JsonAsAssetSettings.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Framework/Text/SlateHyperlinkRun.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include <JsonAsAsset/Public/Utilities/RemoteUtilities.h>

#define LOCTEXT_NAMESPACE "JsonAsAsset"

UJsonAsAssetSettings::UJsonAsAssetSettings() {
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("JsonAsAsset");
}

#if WITH_EDITOR
FText UJsonAsAssetSettings::GetSectionText() const {
	return LOCTEXT("SettingsDisplayName", "JsonAsAsset");
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

	IDetailCategoryBuilder& AssetCategory = DetailBuilder.EditCategory("Behavior", FText::GetEmpty(), ECategoryPriority::Important);

	DetailBuilder.EditCategory("Local Fetch", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch Encryption", FText::GetEmpty(), ECategoryPriority::Important);
	IDetailCategoryBuilder& EncryptionCategory = DetailBuilder.EditCategory("Local Fetch Encryption", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch Director", FText::GetEmpty(), ECategoryPriority::Important);

	EncryptionCategory.AddCustomRow(LOCTEXT("EncryptionKeyGenerator", "EncryptionKeyGenerator"))
		.ValueContent()[
			SNew(SButton)
				.Text(LOCTEXT("GenerateEncryptionKey", "[FORTNITE] Generate Encryption"))
				.ToolTipText(LOCTEXT("GenerateEncryptionKey_Tooltip", "Generates encryption keys from a Fortnite API."))
				.OnClicked_Lambda([this, Settings]() {
				UJsonAsAssetSettings* PluginSettings = GetMutableDefault<UJsonAsAssetSettings>();
				FHttpModule* HttpModule = &FHttpModule::Get();

				const TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
				Request->SetURL("https://fortnitecentral.genxgames.gg/api/v1/aes");
				Request->SetVerb(TEXT("GET"));

				const TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response = FRemoteUtilities::ExecuteRequestSync(Request);
				if (!Response.IsValid()) return FReply::Handled();

				PluginSettings->DynamicKeys.Empty();

				TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				TSharedPtr<FJsonObject> JsonObject;
				if (FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
					PluginSettings->ArchiveKey = JsonObject->GetStringField("mainKey");

					for (const TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("dynamicKeys")) {
						const TSharedPtr<FJsonObject> Object = Value->AsObject();

						FString GUID = Object->GetStringField("guid");
						FString Key = Object->GetStringField("key");

						PluginSettings->DynamicKeys.Add(FParseKey(GUID, Key));
					}
				}

				PluginSettings->SaveConfig();
				PluginSettings->UpdateDefaultConfigFile();
				PluginSettings->LoadConfig();

				FString LocalExportDirectory = PluginSettings->ExportDirectory.Path;

				if (LocalExportDirectory != "" && LocalExportDirectory.Contains("/")) {
					FString DataFolder; {
						if (LocalExportDirectory.EndsWith("/"))
							LocalExportDirectory.Split("/", &DataFolder, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
						else DataFolder = LocalExportDirectory;

						DataFolder.Split("/", &DataFolder, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd); {
							DataFolder = DataFolder + "/.data";
						}
					}

					const TSharedRef<IHttpRequest> MappingsURLRequest = HttpModule->CreateRequest();
					MappingsURLRequest->SetURL("https://fortnitecentral.genxgames.gg/api/v1/mappings");
					MappingsURLRequest->SetVerb(TEXT("GET"));

					const TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> MappingsURLResponse = FRemoteUtilities::ExecuteRequestSync(MappingsURLRequest);
					if (!MappingsURLResponse.IsValid()) return FReply::Handled();

					JsonReader = TJsonReaderFactory<>::Create(MappingsURLResponse->GetContentAsString());
					TArray<TSharedPtr<FJsonValue>> JsonArray;

					if (FJsonSerializer::Deserialize(JsonReader, JsonArray)) {
						TSharedPtr<FJsonValue> Value; {
							if (JsonArray.IsValidIndex(1))
								Value = JsonArray[1];
							else Value = JsonArray[0];
						}

						if (Value.IsValid()) return FReply::Handled();
						TSharedPtr<FJsonObject> MappingsObject = Value->AsObject();

						FString FileName = MappingsObject->GetStringField("fileName");
						FString URL = MappingsObject->GetStringField("url");

						auto OnRequestComplete = [DataFolder, FileName](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
							if (bWasSuccessful) {
								FFileHelper::SaveArrayToFile(Response->GetContent(), *(DataFolder + "/" + FileName));

								UJsonAsAssetSettings* PluginSettings = GetMutableDefault<UJsonAsAssetSettings>();
								PluginSettings->MappingFilePath.FilePath = DataFolder + "/" + FileName;
								PluginSettings->SaveConfig();
								PluginSettings->UpdateDefaultConfigFile();
								PluginSettings->LoadConfig();
							}
						};

						TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> MappingsRequest = FHttpModule::Get().CreateRequest();
						MappingsRequest->SetVerb("GET");
						MappingsRequest->SetURL(URL);
						MappingsRequest->OnProcessRequestComplete().BindLambda(OnRequestComplete);
						MappingsRequest->ProcessRequest();
					}
				}

				return FReply::Handled();
					}).IsEnabled_Lambda([this, Settings]() {
						return Settings->bEnableLocalFetch;
						})
		];
}
#undef LOCTEXT_NAMESPACE