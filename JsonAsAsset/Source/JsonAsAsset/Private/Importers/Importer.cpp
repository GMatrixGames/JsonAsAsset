// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Importer.h"
#include "Settings/JsonAsAssetSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "RemoteAssetDownloader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Json.h"

// ----> Importers
#include "Importers/CurveFloatImporter.h"
#include "Importers/CurveLinearColorImporter.h"
#include "Importers/DataTableImporter.h"
#include "Importers/SkeletalMeshLODSettingsImporter.h"
#include "Importers/SkeletonImporter.h"
#include "Importers/SoundAttenuationImporter.h"
#include "Importers/ReverbEffectImporter.h"
#include "Importers/SubsurfaceProfileImporter.h"
#include "Importers/AnimationBaseImporter.h"
#include "Importers/MaterialFunctionImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/MaterialParameterCollectionImporter.h"
#include "Importers/MaterialInstanceConstantImporter.h"
#include "Importers/TextureImporters.h"
#include "Utilities/AssetUtilities.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/MessageDialog.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "IImporter"

template <typename T>
void IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
	Name = Name.Replace(TEXT("'"), TEXT(""));

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	if (Settings->bTextureRemoteDownload) {
		const UObject* DefaultObject = T::StaticClass()->ClassDefaultObject;

		if (DefaultObject != nullptr) {
			if (DefaultObject->IsA(UTexture::StaticClass()) && !DefaultObject->IsA(UTextureRenderTarget2D::StaticClass())) {
				UTexture2D* Tex;
				if (!FRemoteAssetDownloader::MakeTexture(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").ToString(), Tex)) {
					UE_LOG(LogJson, Log, TEXT("Something went wrong here!!"));
				}

				Object = Cast<T>(Tex);
				return;
			}
		}
	}

	Object = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));
}

bool IImporter::HandleReference(FString GamePath) {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	if (!Settings->bAutomateReferences)
		return false;

	FString UnSanitizedCodeName;
	FilePath.Split(Settings->ExportDirectory.Path + "/", nullptr, &UnSanitizedCodeName);
	UnSanitizedCodeName.Split("/", &UnSanitizedCodeName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);

	// TODO: As of writing this, I don't know how to add Plugin support
	FString UnSanitizedPath;
	UnSanitizedPath = GamePath.Replace(TEXT("/Game/"), *(UnSanitizedCodeName + "/Content/"));
	UnSanitizedPath = (Settings->ExportDirectory.Path + "/") + UnSanitizedPath + ".json";

	FString ContentBefore;
	if (FFileHelper::LoadFileToString(ContentBefore, *UnSanitizedPath)) {
		ImportReference(UnSanitizedPath);

		return true;
	}
	
	return false;
}

void IImporter::ImportReference(FString File) {
	/* ----  Parse JSON into UE JSON Reader ---- */
	FString ContentBefore;
	FFileHelper::LoadFileToString(ContentBefore, *File);

	FString Content = FString(TEXT("{\"data\": "));
	Content.Append(ContentBefore);
	Content.Append(FString("}"));

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
	/* ---------------------------------------- */

	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField("data");

		TArray<FString> Types;
		for (TSharedPtr<FJsonValue>& Obj : DataObjects) Types.Add(Obj->AsObject()->GetStringField("Type"));

		// If type is not supported, decline
		if (!IImporter::CanImportAny(Types)) {
			const FText DialogText = FText::FromString("No exports from \"" + File + "\" can be imported!");
			FMessageDialog::Open(EAppMsgType::Ok, DialogText);
		}

		for (const TSharedPtr<FJsonValue>& Obj : DataObjects) {
			TSharedPtr<FJsonObject> DataObject = Obj->AsObject();

			FString Type = DataObject->GetStringField("Type");
			FString Name = DataObject->GetStringField("Name");

			if (IImporter::CanImport(Type)) {
				// Convert from relative to full
				// NOTE: Used for references
				if (FPaths::IsRelative(File))
					File = FPaths::ConvertRelativePathToFull(File);

				UPackage* LocalOutermostPkg;
				UPackage* LocalPackage = FAssetUtilities::CreateAssetPackage(Name, File, LocalOutermostPkg);
				IImporter* Importer;

				if (Type == "CurveFloat") Importer = new UCurveFloatImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "CurveLinearColor") Importer = new UCurveLinearColorImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "AnimSequence") Importer = new UAnimationBaseImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "Skeleton") Importer = new USkeletonImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, DataObjects);
				else if (Type == "SkeletalMeshLODSettings") Importer = new USkeletalMeshLODSettingsImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "ReverbEffect") Importer = new UReverbEffectImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "SoundAttenuation") Importer = new USoundAttenuationImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "Material") Importer = new UMaterialImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, DataObjects);
				else if (Type == "MaterialFunction") Importer = new UMaterialFunctionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, DataObjects);
				else if (Type == "MaterialInstanceConstant") Importer = new UMaterialInstanceConstantImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, DataObjects);
				else if (Type == "MaterialParameterCollection") Importer = new UMaterialParameterCollectionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, DataObjects);

				else if (Type == "DataTable") Importer = new UDataTableImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "SubsurfaceProfile") Importer = new USubsurfaceProfileImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "TextureRenderTarget2D") Importer = new UTextureImporters(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else Importer = nullptr;

				if (Importer != nullptr && Importer->ImportData()) {
					UE_LOG(LogJson, Log, TEXT("Successfully imported \"%s\" as \"%s\""), *Name, *Type);

					// Notification for asset
					AppendNotification(
						FText::FromString("Imported type: " + Type),
						FText::FromString(Name),
						2.0f,
						FSlateIconFinder::FindCustomIconBrushForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Type)), TEXT("ClassThumbnail")),
						SNotificationItem::CS_Success,
						false,
						350.0f
					);
				}
				else {
					FText DialogText = FText::FromString("The \"" + Type + "\" cannot be imported!");
					FMessageDialog::Open(EAppMsgType::Ok, DialogText);
				}
			}
		}
	}
}

bool IImporter::HandleAssetCreation(UObject* Asset) {
	FAssetRegistryModule::AssetCreated(Asset);
	if (!Asset->MarkPackageDirty()) return false;
	Package->SetDirtyFlag(true);
	Asset->PostEditChange();
	Asset->AddToRoot();

	// Browse to newly added Asset
	const TArray<FAssetData>& Assets = { Asset };
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(Assets);

	return true;
}

FName IImporter::GetExportNameOfSubobject(const FString& PackageIndex) {
	FString Name;
	PackageIndex.Split("'", nullptr, &Name);
	Name.Split(":", nullptr, &Name);
	Name = Name.Replace(TEXT("'"), TEXT(""));
	return FName(Name);
}

TArray<TSharedPtr<FJsonValue>> IImporter::FilterExportsByOuter(const FString& Outer) {
	TArray<TSharedPtr<FJsonValue>> ReturnValue = TArray<TSharedPtr<FJsonValue>>();

	for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
		const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Value->AsObject());

		FString ExOuter;
		if (ValueObject->TryGetStringField("Outer", ExOuter) && ExOuter == Outer) {
			ReturnValue.Add(TSharedPtr(Value));
		}
	}

	return ReturnValue;
}

TSharedPtr<FJsonValue> IImporter::GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object) {
	const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Object);

	FString StringIndex;
	ValueObject->GetStringField("ObjectPath").Split(".", nullptr, &StringIndex);

	return AllJsonObjects[FCString::Atod(*StringIndex)];
}

void IImporter::AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons, float WidthOverride) {
	FNotificationInfo Info = FNotificationInfo(Text);
	Info.ExpireDuration = ExpireDuration;
	Info.bUseLargeFont = true;
	Info.bUseSuccessFailIcons = bUseSuccessFailIcons;
	Info.WidthOverride = FOptionalSize(WidthOverride);
	Info.SubText = SubText;

	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(CompletionState);
}

void IImporter::AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, const FSlateBrush* SlateBrush, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons, float WidthOverride) {
	FNotificationInfo Info = FNotificationInfo(Text);
	Info.ExpireDuration = ExpireDuration;
	Info.bUseLargeFont = true;
	Info.bUseSuccessFailIcons = bUseSuccessFailIcons;
	Info.WidthOverride = FOptionalSize(WidthOverride);
	Info.SubText = SubText;
	Info.Image = SlateBrush;

	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(CompletionState);
}

#undef LOCTEXT_NAMESPACE