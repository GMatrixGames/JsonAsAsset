// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Importer.h"
#include "Settings/JsonAsAssetSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"

// ----> Importers
#include "Importers/CurveFloatImporter.h"
#include "Importers/CurveVectorImporter.h"
#include "Importers/CurveLinearColorImporter.h"
#include "Importers/CurveLinearColorAtlasImporter.h"
#include "Importers/DataTableImporter.h"
#include "Importers/SkeletalMeshLODSettingsImporter.h"
#include "Importers/SkeletonImporter.h"
#include "Importers/SoundAttenuationImporter.h"
#include "Importers/SoundConcurrencyImporter.h"
#include "Importers/ReverbEffectImporter.h"
#include "Importers/SubsurfaceProfileImporter.h"
#include "Importers/AnimationBaseImporter.h"
#include "Importers/LandscapeGrassTypeImporter.h"
#include "Importers/MaterialFunctionImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/MaterialParameterCollectionImporter.h"
#include "Importers/MaterialInstanceConstantImporter.h"
#include "Importers/PhysicalMaterialImporter.h"
#include "Importers/TextureImporters.h"
#include "Utilities/AssetUtilities.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/MessageDialog.h"
#include "Misc/FileHelper.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "IImporter"

template <typename T>
TObjectPtr<T> IImporter::DownloadWrapper(TObjectPtr<T> InObject, FString Type, FString Name, FString Path) {
	// If the asset can be found locally
	if (InObject == nullptr && HandleReference(Path)) {
		TObjectPtr<T> Object = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));

		return Object;
	}

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	bool bRemoteDownload = Settings->bEnableRemoteDownload;

	FMessageLog MessageLogger = FMessageLog(FName("JsonAsAsset"));

	if (bRemoteDownload && InObject == nullptr) {
		const UObject* DefaultObject = T::StaticClass()->ClassDefaultObject;

		if (DefaultObject != nullptr) {
			bool bRemoteDownloadStatus = false;
			bool bTriedDownload = false;

			bTriedDownload = FAssetUtilities::ConstructAsset(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").ToString(), Type, InObject, bRemoteDownloadStatus);

			// Notification
			if (bTriedDownload) {
				if (bRemoteDownloadStatus) {
					AppendNotification(
						FText::FromString("Remotely Downloaded: " + Type),
						FText::FromString(Name),
						2.0f,
						FSlateIconFinder::FindCustomIconBrushForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Type)), TEXT("ClassThumbnail")),
						SNotificationItem::CS_Success,
						false,
						310.0f
					);

					MessageLogger.Message(EMessageSeverity::Info, FText::FromString("Downloaded asset: " + Name + " (" + Type + ")"));
				} else {
					AppendNotification(
						FText::FromString("Download Failed: " + Type),
						FText::FromString(Name),
						5.0f,
						FSlateIconFinder::FindCustomIconBrushForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Type)), TEXT("ClassThumbnail")),
						SNotificationItem::CS_Fail,
						false,
						310.0f
					);

					MessageLogger.Error(FText::FromString("Failed to download asset: " + Name + " (" + Type + ")"));
				}
			}
		}
	}

	// Gotta let em know
	if (InObject == nullptr)
		MessageLogger.Error(FText::FromString("Object not found while importing: " + Name + " (" + Type + ")"));

	return InObject;
}

template <typename T>
void IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
	Name = Name.Replace(TEXT("'"), TEXT(""));

	// Define found object
	TObjectPtr<T> Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));

	Object = DownloadWrapper(Obj, Type, Name, Path);
}

template <typename T>
TArray<TObjectPtr<T>> IImporter::LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array) {
	// Go through each
	for (const TSharedPtr<FJsonValue> ArrayElement : PackageArray) {
		const TSharedPtr<FJsonObject> Ptr = ArrayElement->AsObject();

		FString Type;
		FString Name;
		Ptr->GetStringField("ObjectName").Split("'", &Type, &Name);
		FString Path;
		Ptr->GetStringField("ObjectPath").Split(".", &Path, nullptr);
		Name = Name.Replace(TEXT("'"), TEXT(""));

		TObjectPtr<T> Object = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));

		Array.Add(DownloadWrapper(Object, Type, Name, Path));
	}

	return Array;
}

bool IImporter::HandleReference(const FString& GamePath) {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	FString UnSanitizedCodeName;
	FilePath.Split(Settings->ExportDirectory.Path + "/", nullptr, &UnSanitizedCodeName);
	UnSanitizedCodeName.Split("/", &UnSanitizedCodeName, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);

	// TODO: As of writing this, I don't know how to add Plugin support
	FString UnSanitizedPath = GamePath.Replace(TEXT("/Game/"), *(UnSanitizedCodeName + "/Content/"));
	UnSanitizedPath = Settings->ExportDirectory.Path + "/" + UnSanitizedPath + ".json";

	FString ContentBefore;
	if (FFileHelper::LoadFileToString(ContentBefore, *UnSanitizedPath)) {
		ImportReference(UnSanitizedPath);

		return true;
	}

	return false;
}

void IImporter::ImportReference(const FString& File) {
	/* ----  Parse JSON into UE JSON Reader ---- */
	FString ContentBefore;
	FFileHelper::LoadFileToString(ContentBefore, *File);

	FString Content = FString(TEXT("{\"data\": "));
	Content.Append(ContentBefore);
	Content.Append(FString("}"));

	TSharedPtr<FJsonObject> JsonParsed;
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
	/* ---------------------------------------- */

	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		const TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField("data");

		HandleExports(DataObjects, File);
	}
}

bool IImporter::HandleAssetCreation(UObject* Asset) const {
	FAssetRegistryModule::AssetCreated(Asset);
	if (!Asset->MarkPackageDirty()) return false;
	Package->SetDirtyFlag(true);
	Asset->PostEditChange();
	Asset->AddToRoot();

	// Browse to newly added Asset
	const TArray<FAssetData>& Assets = {Asset};
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(Assets);

	return true;
}

bool IImporter::HandleExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, const bool bHideNotifications) {
	TArray<FString> Types;
	for (const TSharedPtr<FJsonValue>& Obj : Exports) Types.Add(Obj->AsObject()->GetStringField("Type"));

	// If type is not supported, decline
	if (!CanImportAny(Types)) {
		const FText DialogText = FText::FromString("No exports from \"" + File + "\" can be imported!");
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	}

	for (const TSharedPtr<FJsonValue>& ExportPtr : Exports) {
		TSharedPtr<FJsonObject> DataObject = ExportPtr->AsObject();

		FString Type = DataObject->GetStringField("Type");
		FString Name = DataObject->GetStringField("Name");

		if (CanImport(Type)) {
			// Convert from relative to full
			// NOTE: Used for references
			if (FPaths::IsRelative(File)) File = FPaths::ConvertRelativePathToFull(File);

			UPackage* LocalOutermostPkg;
			UPackage* LocalPackage = FAssetUtilities::CreateAssetPackage(Name, File, LocalOutermostPkg);
			IImporter* Importer;

			if (Type == "CurveFloat") Importer = new UCurveFloatImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "CurveVector") Importer = new UCurveVectorImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "CurveLinearColor") Importer = new UCurveLinearColorImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "CurveLinearColorAtlas") Importer = new UCurveLinearColorAtlasImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "AnimSequence") Importer = new UAnimationBaseImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

			else if (Type == "Skeleton") Importer = new USkeletonImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
			else if (Type == "SkeletalMeshLODSettings") Importer = new USkeletalMeshLODSettingsImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

			else if (Type == "ReverbEffect") Importer = new UReverbEffectImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "SoundAttenuation") Importer = new USoundAttenuationImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "SoundConcurrency") Importer = new USoundConcurrencyImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

			else if (Type == "Material") Importer = new UMaterialImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
			else if (Type == "MaterialFunction") Importer = new UMaterialFunctionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
			else if (Type == "MaterialInstanceConstant") Importer = new UMaterialInstanceConstantImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
			else if (Type == "MaterialParameterCollection") Importer = new UMaterialParameterCollectionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
			else if (Type == "PhysicalMaterial") Importer = new UPhysicalMaterialImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

			else if (Type == "LandscapeGrassType") Importer = new ULandscapeGrassTypeImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

			else if (Type == "DataTable") Importer = new UDataTableImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "SubsurfaceProfile") Importer = new USubsurfaceProfileImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else if (Type == "TextureRenderTarget2D") Importer = new UTextureImporters(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
			else Importer = nullptr;

			FMessageLog MessageLogger = FMessageLog(FName("JsonAsAsset"));

			if (bHideNotifications) {
				Importer->ImportData();

				return true;
			}

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

				MessageLogger.Message(EMessageSeverity::Info, FText::FromString("Imported Asset: " + Name + " (" + Type + ")"));
			} else FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("The \"" + Type + "\" cannot be imported!"));
		}
	}

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

		if (FString ExOuter; ValueObject->TryGetStringField("Outer", ExOuter) && ExOuter == Outer) ReturnValue.Add(TSharedPtr(Value));
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

	const TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
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

	const TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(CompletionState);
}

#undef LOCTEXT_NAMESPACE
