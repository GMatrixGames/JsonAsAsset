// Copyright JAA Contributors 2024-2025

#include "JsonAsAssetSettingsDetails.h"
#include "Settings/JsonAsAssetSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Utilities/RemoteUtilities.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"

#define LOCTEXT_NAMESPACE "JsonAsAssetSettingsDetails"

TSharedRef<IDetailCustomization> FJsonAsAssetSettingsDetails::MakeInstance()
{
	return MakeShareable(new FJsonAsAssetSettingsDetails);
}

void FJsonAsAssetSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Reference to settings
	TWeakObjectPtr<UJsonAsAssetSettings> Settings = Cast<UJsonAsAssetSettings>(ObjectsBeingCustomized[0].Get());

	IDetailCategoryBuilder& AssetCategory = DetailBuilder.EditCategory("Configuration", FText::GetEmpty(), ECategoryPriority::Important);

	AssetCategory.AddCustomRow(LOCTEXT("UseFModelAppSettings", "UseFModelAppSettings"))
    .NameContent()
    [
        // This defines the name/title of the row
        SNew(STextBlock)
        .Text(LOCTEXT("UseFModelAppSettings", "Load External Configuration"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    [
        // Now we define the value/content of the row
        SNew(SButton)
        .Text(LOCTEXT("UseFModelAppSettings_Text", "FModel Settings"))
        .ToolTipText(LOCTEXT("UseFModelAppSettings_Tooltip", "Takes AppData/Roaming/FModel/AppSettings.json settings and sets them here"))
        .OnClicked_Lambda([this, Settings]()
        {
            UJsonAsAssetSettings* PluginSettings = GetMutableDefault<UJsonAsAssetSettings>();

            // Get the path to AppData\Roaming
            FString AppDataPath = FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA"));
            AppDataPath = FPaths::Combine(AppDataPath, TEXT("FModel/AppSettings.json"));

            if (FString JsonContent; FFileHelper::LoadFileToString(JsonContent, *AppDataPath)) {
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

                if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) {
                    // Load the PropertiesDirectory and GameDirectory
                    PluginSettings->ExportDirectory.Path = JsonObject->GetStringField("PropertiesDirectory").Replace(TEXT("\\"), TEXT("/"));
                    PluginSettings->ArchiveDirectory.Path = JsonObject->GetStringField("GameDirectory").Replace(TEXT("\\"), TEXT("/"));

					FString GameDirectory = JsonObject->GetStringField("GameDirectory");

                    // Handling AES Keys
                    if (JsonObject->HasField("PerDirectory")) {
                        const TSharedPtr<FJsonObject> PerDirectoryObject = JsonObject->GetObjectField("PerDirectory");

                        if (PerDirectoryObject->HasField(GameDirectory)) {
                            const TSharedPtr<FJsonObject> PakSettings = PerDirectoryObject->GetObjectField(GameDirectory);

                            if (PakSettings->HasField("AesKeys")) {
                                const TSharedPtr<FJsonObject> AesKeysObject = PakSettings->GetObjectField("AesKeys");

                                if (AesKeysObject->HasField("mainKey")) {
                                    PluginSettings->ArchiveKey = AesKeysObject->GetStringField("mainKey");
                                }

                                if (AesKeysObject->HasField("dynamicKeys")) {
                                    const TArray<TSharedPtr<FJsonValue>> DynamicKeysArray = AesKeysObject->GetArrayField("dynamicKeys");
                                    PluginSettings->DynamicKeys.Empty();

                                    for (const auto& KeyValue : DynamicKeysArray) {
                                        const TSharedPtr<FJsonObject> KeyObject = KeyValue->AsObject();

                                        if (KeyObject.IsValid()) {
                                            FParseKey NewKey;
                                            NewKey.Guid = KeyObject->GetStringField("guid");
                                            NewKey.Value = KeyObject->GetStringField("key");
                                            PluginSettings->DynamicKeys.Add(NewKey);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            PluginSettings->SaveConfig();
            PluginSettings->UpdateDefaultConfigFile();
            PluginSettings->LoadConfig();

            return FReply::Handled();
        })
    ];

	DetailBuilder.EditCategory("Local Fetch", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch - Configuration", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch - Encryption", FText::GetEmpty(), ECategoryPriority::Important);

	IDetailCategoryBuilder& EncryptionCategory = DetailBuilder.EditCategory("Local Fetch - Encryption", FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Local Fetch - Director", FText::GetEmpty(), ECategoryPriority::Important);

	EncryptionCategory.AddCustomRow(LOCTEXT("EncryptionKeyFetcher", "EncryptionKeyFetcher"))
		.NameContent()
		[
			// This defines the name/title of the row
			SNew(STextBlock)
				.Text(LOCTEXT("UseFModelAppSettings", "Fetches Encryption from a API"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()[
		SNew(SButton)
		.Text(LOCTEXT("FetchEncryptionKey", "Fortnite Central API"))
		.ToolTipText(LOCTEXT("FetchEncryptionKey_Tooltip", "Fetches encryption keys from the Fortnite Central API"))
		.OnClicked_Lambda([this, Settings]
		{
			UJsonAsAssetSettings* PluginSettings = GetMutableDefault<UJsonAsAssetSettings>();
			FHttpModule* HttpModule = &FHttpModule::Get();

			const TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
			Request->SetURL("https://fortnitecentral.genxgames.gg/api/v1/aes");
			Request->SetVerb(TEXT("GET"));

			const TSharedPtr<IHttpResponse> Response = FRemoteUtilities::ExecuteRequestSync(Request);
			if (!Response.IsValid()) return FReply::Handled();

			PluginSettings->DynamicKeys.Empty();

			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(JsonReader, JsonObject))
			{
				PluginSettings->ArchiveKey = JsonObject->GetStringField("mainKey");

				for (const TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("dynamicKeys"))
				{
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

			if (LocalExportDirectory != "" && LocalExportDirectory.Contains("/"))
			{
				FString DataFolder;
				{
					if (LocalExportDirectory.EndsWith("/")) LocalExportDirectory.Split("/", &DataFolder, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					else DataFolder = LocalExportDirectory;

					DataFolder.Split("/", &DataFolder, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					{
						DataFolder = DataFolder + "/.data";
					}
				}

				const TSharedRef<IHttpRequest> MappingsURLRequest = HttpModule->CreateRequest();
				MappingsURLRequest->SetURL("https://fortnitecentral.genxgames.gg/api/v1/mappings");
				MappingsURLRequest->SetVerb(TEXT("GET"));

				const TSharedPtr<IHttpResponse> MappingsURLResponse = FRemoteUtilities::ExecuteRequestSync(MappingsURLRequest);
				if (!MappingsURLResponse.IsValid()) return FReply::Handled();

				JsonReader = TJsonReaderFactory<>::Create(MappingsURLResponse->GetContentAsString());
				if (TArray<TSharedPtr<FJsonValue>> JsonArray; FJsonSerializer::Deserialize(JsonReader, JsonArray))
				{
					TSharedPtr<FJsonValue> Value;
					{
						if (JsonArray.IsValidIndex(1)) Value = JsonArray[1];
						else Value = JsonArray[0];
					}

					if (Value == nullptr) return FReply::Handled();
					TSharedPtr<FJsonObject> MappingsObject = Value->AsObject();

					FString FileName = MappingsObject->GetStringField("fileName");
					FString URL = MappingsObject->GetStringField("url");

					auto OnRequestComplete = [DataFolder, FileName](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
					{
						if (bWasSuccessful)
						{
							FFileHelper::SaveArrayToFile(Response->GetContent(), *(DataFolder + "/" + FileName));

							UJsonAsAssetSettings* PluginSettings = GetMutableDefault<
								UJsonAsAssetSettings>();
							PluginSettings->MappingFilePath.FilePath = DataFolder + "/" + FileName;
							PluginSettings->SaveConfig();
							PluginSettings->UpdateDefaultConfigFile();
							PluginSettings->LoadConfig();
						}
					};

					TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MappingsRequest = FHttpModule::Get().
						CreateRequest();
					MappingsRequest->SetVerb("GET");
					MappingsRequest->SetURL(URL);
					MappingsRequest->OnProcessRequestComplete().BindLambda(OnRequestComplete);
					MappingsRequest->ProcessRequest();

					PluginSettings->SaveConfig();
					PluginSettings->UpdateDefaultConfigFile();
					PluginSettings->LoadConfig();
				}
			}

			return FReply::Handled();
		}).IsEnabled_Lambda([this, Settings]()
		{
			return Settings->bEnableLocalFetch;
		})
	];
}

#undef LOCTEXT_NAMESPACE