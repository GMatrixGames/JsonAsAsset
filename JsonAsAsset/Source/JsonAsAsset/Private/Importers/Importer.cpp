// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Importer.h"
#include "CoreMinimal.h"

#include "Settings/JsonAsAssetSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Json.h"

// ----> Importers
#include "Importers/CurveFloatImporter.h"
#include "Importers/CurveVectorImporter.h"
#include "Importers/CurveLinearColorImporter.h"
#include "Importers/CurveLinearColorAtlasImporter.h"
#include "Importers/DataTableImporter.h"
#include "Importers/SkeletalMeshLODSettingsImporter.h"
#include "Importers/SkeletonImporter.h"
#include "Importers/SoundAttenuationImporter.h"
#include "Importers/PhysicsImporter.h"
#include "Importers/SoundConcurrencyImporter.h"
#include "Importers/ReverbEffectImporter.h"
#include "Importers/SubsurfaceProfileImporter.h"
#include "Importers/AnimationBaseImporter.h"
#include "Importers/LandscapeGrassTypeImporter.h"
#include "Importers/MaterialFunctionImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/MaterialParameterCollectionImporter.h"
#include "Importers/NiagaraParameterCollectionImporter.h"
#include "Importers/MaterialInstanceConstantImporter.h"
#include "Importers/SoundCueImporter.h"
#include "Importers/PhysicalMaterialImporter.h"
#include "Importers/TextureImporter.h"
#include "Importers/DataAssetImporter.h"
// <---- Importers

#include "Utilities/AssetUtilities.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Importers/CurveTableImporter.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/MessageDialog.h"
#include "Misc/FileHelper.h"
#include "Logging/MessageLog.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "IImporter"

template <typename T>
TObjectPtr<T> IImporter::DownloadWrapper(TObjectPtr<T> InObject, FString Type, FString Name, FString Path) {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	// If the asset can be found locally
	if (InObject == nullptr && HandleReference(Path)) {
		TObjectPtr<T> Object = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));

		return Object;
	}

	bool bEnableLocalFetch = Settings->bEnableLocalFetch;
	FMessageLog MessageLogger = FMessageLog(FName("JsonAsAsset"));

	if (bEnableLocalFetch && (
		InObject == nullptr ||
			Settings->bDownloadExistingTextures &&
			Type == "Texture2D"
		)
	) {
		const UObject* DefaultObject = T::StaticClass()->ClassDefaultObject;

		if (DefaultObject != nullptr) {
			bool bRemoteDownloadStatus = false;
			bool bTriedDownload = false;

			bTriedDownload = FAssetUtilities::ConstructAsset(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").ToString(), Type, InObject, bRemoteDownloadStatus);

			// Notification
			if (bTriedDownload) {
				if (bRemoteDownloadStatus) {
					AppendNotification(
						FText::FromString("Locally Downloaded: " + Type),
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

	return InObject;
}

template <typename T>
void IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	Path = Path.Replace(TEXT("FortniteGame/Content"), TEXT("/Game"));
	Path = Path.Replace(TEXT("Engine/Content"), TEXT("/Engine"));

	Name = Name.Replace(TEXT("'"), TEXT(""));

#pragma warning( push )
#pragma warning( disable : 4101) // Hide LoadObject Fail
	// Define found object
	TObjectPtr<T> Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));

	FString DirectString;
	Settings->RedirectFolderDirectory.Path.Split("/", &DirectString, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	if (Obj == nullptr && Settings->bEnableModifications) {
		if (Path.StartsWith("/Game/"))
			DirectString = Settings->RedirectFolderDirectory.Path;

		Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(DirectString + Path.Replace(TEXT("/Game/"), TEXT("")) + "." + Name)));

		if (Obj == nullptr && !Path.StartsWith("/Game/"))
			Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(DirectString + "/Plugins" + Path.Replace(TEXT("/Game/"), TEXT("")) + "." + Name)));
	}

	// Material Expressions may be formatted differently
	if (Obj == nullptr && Name.Contains("MaterialExpression")) {
		FString AssetName; 
			Path.Split("/", nullptr, &AssetName, ESearchCase::Type::IgnoreCase, ESearchDir::FromEnd);

		// Load Object with :
		Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + AssetName + ":" + Name)));

		if (Obj == nullptr && Settings->bEnableModifications) // Fix references to plugins
			Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(DirectString + Path.Replace(TEXT("/Game/"), TEXT("")) + "." + AssetName + ":" + Name)));
		if (Obj == nullptr && !Path.StartsWith("/Game/") && Settings->bEnableModifications)
			Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(DirectString + "/Plugins" + Path.Replace(TEXT("/Game/"), TEXT("")) + "." + Name)));
	}
#pragma warning( pop )
	
	Object = DownloadWrapper(Obj, Type, Name, Path);
}

template <typename T>
TArray<TObjectPtr<T>> IImporter::LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array) {
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
	Package->FullyLoad();

	// Browse to newly added Asset
	const TArray<FAssetData>& Assets = {Asset};
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(Assets);

	return true;
}

void IImporter::SavePackage() {
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	Package->FullyLoad();

	FSavePackageArgs SaveArgs; {
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
	}

	const FString PackageName = Package->GetName();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	if (Settings->bAllowPackageSaving)
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
}

bool IImporter::HandleExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, const bool bHideNotifications) {
	TArray<FString> Types;
	for (const TSharedPtr<FJsonValue>& Obj : Exports) Types.Add(Obj->AsObject()->GetStringField("Type"));

	for (const TSharedPtr<FJsonValue>& ExportPtr : Exports) {
		TSharedPtr<FJsonObject> DataObject = ExportPtr->AsObject();

		FString Type = DataObject->GetStringField("Type");
		FString Name = DataObject->GetStringField("Name");

		UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Type);

		if (Class == nullptr) continue;
		bool bDataAsset = Class->IsChildOf(UDataAsset::StaticClass());

		if (CanImport(Type) || bDataAsset) {
			// Convert from relative to full
			// NOTE: Used for references
			if (FPaths::IsRelative(File)) File = FPaths::ConvertRelativePathToFull(File);

			IImporter* Importer;
			if (Type == "AnimSequence" || Type == "AnimMontage") 
				Importer = new UAnimationBaseImporter(Name, File, DataObject, nullptr, nullptr);
			else {
				UPackage* LocalOutermostPkg;
				UPackage* LocalPackage = FAssetUtilities::CreateAssetPackage(Name, File, LocalOutermostPkg);

				if (Type == "CurveFloat") Importer = new UCurveFloatImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "CurveTable") Importer = new UCurveTableImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "CurveVector") Importer = new UCurveVectorImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "CurveLinearColor") Importer = new UCurveLinearColorImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "CurveLinearColorAtlas") Importer = new UCurveLinearColorAtlasImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "Skeleton") Importer = new USkeletonImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "SkeletalMeshLODSettings") Importer = new USkeletalMeshLODSettingsImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "SoundCue") Importer = new USoundCueImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "ReverbEffect") Importer = new UReverbEffectImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "SoundAttenuation") Importer = new USoundAttenuationImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "SoundConcurrency") Importer = new USoundConcurrencyImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "Material") Importer = new UMaterialImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "MaterialFunction") Importer = new UMaterialFunctionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "MaterialInstanceConstant") Importer = new UMaterialInstanceConstantImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "MaterialParameterCollection") Importer = new UMaterialParameterCollectionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "PhysicalMaterial") Importer = new UPhysicalMaterialImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);

				else if (Type == "LandscapeGrassType") Importer = new ULandscapeGrassTypeImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "NiagaraParameterCollection") Importer = new UNiagaraParameterCollectionImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "PhysicsAsset") Importer = new UPhysicsImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "DataTable") Importer = new UDataTableImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (Type == "SubsurfaceProfile") Importer = new USubsurfaceProfileImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg);
				else if (bDataAsset) Importer = new UDataAssetImporter(Class, Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else Importer = nullptr;
			}

			FMessageLog MessageLogger = FMessageLog(FName("JsonAsAsset"));

			if (bHideNotifications) {
				Importer->ImportData();

				return true;
			}

			if (Importer != nullptr && Importer->ImportData()) {
				UE_LOG(LogJson, Log, TEXT("Successfully imported \"%s\" as \"%s\""), *Name, *Type);
				
				if (!(Type == "AnimSequence" || Type == "AnimMontage"))
					Importer->SavePackage();

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
			} else AppendNotification(
				FText::FromString("Import Failed: " + Type),
				FText::FromString(Name),
				2.0f,
				FSlateIconFinder::FindCustomIconBrushForClass(FindObject<UClass>(nullptr, *("/Script/Engine." + Type)), TEXT("ClassThumbnail")),
				SNotificationItem::CS_Fail,
				false,
				350.0f
			);
		}
	}

	return true;
}

TSharedPtr<FJsonObject> IImporter::GetExport(FJsonObject* PackageIndex) {
	FString ObjectName = PackageIndex->GetStringField("ObjectName"); // Class'Asset:ExportName'
	FString ObjectPath = PackageIndex->GetStringField("ObjectPath"); // Path/Asset.Index

	// Class'Asset:ExportName' --> Asset:ExportName
	ObjectName.Split("'", nullptr, &ObjectName);
	ObjectName.Split("'", &ObjectName, nullptr);

	// Asset:ExportName --> ExportName
	if (ObjectName.Contains(":"))
		ObjectName.Split(":", nullptr, &ObjectName);

	// Path/Asset.Index --> Index
	ObjectPath.Split(".", nullptr, &ObjectPath);

	int ExportIndex = FCString::Atoi(*ObjectPath);

	for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
		const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Value->AsObject());

		if (FString Name; ValueObject->TryGetStringField("Name", Name) && Name == ObjectName)
			return ValueObject;
	}

	return nullptr;
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

		if (FString ExOuter; ValueObject->TryGetStringField("Outer", ExOuter) && ExOuter == Outer) 
			ReturnValue.Add(TSharedPtr(Value));
	}

	return ReturnValue;
}

TSharedPtr<FJsonValue> IImporter::GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object) {
	const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Object);

	FString StringIndex; {
		ValueObject->GetStringField("ObjectPath").Split(".", nullptr, &StringIndex);
	}

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
