// Copyright JAA Contributors 2024-2025

#include "Importers/Constructor/Importer.h"

#include "Settings/JsonAsAssetSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"

// ----> Importers
#include "Importers/Types/CurveFloatImporter.h"
#include "Importers/Types/CurveVectorImporter.h"
#include "Importers/Types/CurveLinearColorImporter.h"
#include "Importers/Types/CurveLinearColorAtlasImporter.h"
#include "Importers/Types/DataTableImporter.h"
#include "Importers/Types/SkeletalMeshLODSettingsImporter.h"
#include "Importers/Types/SkeletonImporter.h"
#include "Importers/Types/SoundAttenuationImporter.h"
#include "Importers/Types/PhysicsImporter.h"
#include "Importers/Types/SoundConcurrencyImporter.h"
#include "Importers/Types/ReverbEffectImporter.h"
#include "Importers/Types/SubsurfaceProfileImporter.h"
#include "Importers/Types/AnimationBaseImporter.h"
#include "Importers/Types/LandscapeGrassTypeImporter.h"
#include "Importers/Types/MaterialFunctionImporter.h"
#include "Importers/Types/MaterialImporter.h"
#include "Importers/Types/MaterialParameterCollectionImporter.h"
#include "Importers/Types/NiagaraParameterCollectionImporter.h"
#include "Importers/Types/MaterialInstanceConstantImporter.h"
#include "Importers/Types/SoundCueImporter.h"
#include "Importers/Types/PhysicalMaterialImporter.h"
#include "Importers/Types/TextureImporter.h"
#include "Importers/Types/DataAssetImporter.h"
#include "Importers/Types/CurveTableImporter.h"
#include "Importers/Types/SoundClassImporter.h"
#include "Importers/Types/SoundMixImporter.h"
#include "Importers/Types/SoundModulationPatchImporter.h"
// <---- Importers


#include "Engine/SubsurfaceProfile.h"
#include "Materials/MaterialParameterCollection.h"
#include "Sound/SoundNode.h"
#include "Curves/CurveLinearColor.h"

#include "Utilities/AssetUtilities.h"
#include "Misc/MessageDialog.h"
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

template void IImporter::LoadObject<UMaterialInterface>(const TSharedPtr<FJsonObject>*, TObjectPtr<UMaterialInterface>&);
template void IImporter::LoadObject<USubsurfaceProfile>(const TSharedPtr<FJsonObject>*, TObjectPtr<USubsurfaceProfile>&);
template void IImporter::LoadObject<UTexture>(const TSharedPtr<FJsonObject>*, TObjectPtr<UTexture>&);
template void IImporter::LoadObject<UMaterialParameterCollection>(const TSharedPtr<FJsonObject>*, TObjectPtr<UMaterialParameterCollection>&);
template void IImporter::LoadObject<UAnimSequence>(const TSharedPtr<FJsonObject>*, TObjectPtr<UAnimSequence>&);
template void IImporter::LoadObject<USoundWave>(const TSharedPtr<FJsonObject>*, TObjectPtr<USoundWave>&);
template void IImporter::LoadObject<UObject>(const TSharedPtr<FJsonObject>*, TObjectPtr<UObject>&);
template void IImporter::LoadObject<UMaterialFunctionInterface>(const TSharedPtr<FJsonObject>*, TObjectPtr<UMaterialFunctionInterface>&);
template void IImporter::LoadObject<USoundNode>(const TSharedPtr<FJsonObject>*, TObjectPtr<USoundNode>&);

template <typename T>
void IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField(TEXT("ObjectName")).Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField(TEXT("ObjectPath")).Split(".", &Path, nullptr);

	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

	if (Settings->AssetSettings.GameName != "") {
		Path = Path.Replace(*(Settings->AssetSettings.GameName + "/Content"), TEXT("/Game"));
	}

	Path = Path.Replace(TEXT("Engine/Content"), TEXT("/Engine"));

	Name = Name.Replace(TEXT("'"), TEXT(""));

#pragma warning( push )
#pragma warning( disable : 4101) // Hide LoadObject Fail
	// Define found object
	TObjectPtr<T> Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + Name)));

	// Material Expressions may be formatted differently
	if (Obj == nullptr && Name.Contains("MaterialExpression")) {
		FString AssetName; 
			Path.Split("/", nullptr, &AssetName, ESearchCase::Type::IgnoreCase, ESearchDir::FromEnd);

		// Load Object with :
		Obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *(Path + "." + AssetName + ":" + Name)));
	}
#pragma warning( pop )
	
	Object = DownloadWrapper(Obj, Type, Name, Path);
}

template TArray<TObjectPtr<UCurveLinearColor>> IImporter::LoadObject<UCurveLinearColor>(const TArray<TSharedPtr<FJsonValue>>&, TArray<TObjectPtr<UCurveLinearColor>>);

template <typename T>
TArray<TObjectPtr<T>> IImporter::LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array) {
	for (const TSharedPtr<FJsonValue> ArrayElement : PackageArray) {
		const TSharedPtr<FJsonObject> Ptr = ArrayElement->AsObject();

		FString Type;
		FString Name;
		Ptr->GetStringField(TEXT("ObjectName")).Split("'", &Type, &Name);
		FString Path;
		Ptr->GetStringField(TEXT("ObjectPath")).Split(".", &Path, nullptr);
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
		const TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField(TEXT("data"));

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
	//Package->FullyLoad(); //Causes crashes on Anim Curve Import didn't get any issue commenting this

	FSavePackageArgs SaveArgs; {
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.SaveFlags = SAVE_NoError;
	}

	if(Package == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Package is null"));
		return;
	}
	const FString PackageName = Package->GetName();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	if (Settings->AssetSettings.bSavePackagesOnImport)
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
}

bool IImporter::HandleExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, const bool bHideNotifications) {
	TArray<FString> Types;
	for (const TSharedPtr<FJsonValue>& Obj : Exports) Types.Add(Obj->AsObject()->GetStringField(TEXT("Type")));

	for (const TSharedPtr<FJsonValue>& ExportPtr : Exports) {
		TSharedPtr<FJsonObject> DataObject = ExportPtr->AsObject();

		FString Type = DataObject->GetStringField(TEXT("Type"));
		FString Name = DataObject->GetStringField(TEXT("Name"));

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
				else if (Type == "SoundClass") Importer = new USoundClassImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "SoundMix") Importer = new USoundMixImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);
				else if (Type == "SoundModulationPatch") Importer = new USoundModulationPatchImporter(Name, File, DataObject, LocalPackage, LocalOutermostPkg, Exports);

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
	FString ObjectName = PackageIndex->GetStringField(TEXT("ObjectName")); // Class'Asset:ExportName'
	FString ObjectPath = PackageIndex->GetStringField(TEXT("ObjectPath")); // Path/Asset.Index

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

		if (FString Name; ValueObject->TryGetStringField(TEXT("Name"), Name) && Name == ObjectName)
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

		if (FString ExOuter; ValueObject->TryGetStringField(TEXT("Outer"), ExOuter) && ExOuter == Outer) 
			ReturnValue.Add(TSharedPtr(Value));
	}

	return ReturnValue;
}

TSharedPtr<FJsonValue> IImporter::GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object) {
	const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Object);

	FString StringIndex; {
		ValueObject->GetStringField(TEXT("ObjectPath")).Split(".", nullptr, &StringIndex);
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
