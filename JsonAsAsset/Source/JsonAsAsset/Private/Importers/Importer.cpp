// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Importer.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "RemoteAssetDownloader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/TextureRenderTarget2D.h"

template <class T>
void IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T> Object) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
	Name = Name.Replace(TEXT("'"), TEXT(""));

	if (Object->IsA(UTexture::StaticClass()) && !Object->IsA(UTextureRenderTarget2D::StaticClass())) {
		UTexture2D* O;
		if (!FRemoteAssetDownloader::MakeTexture(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").ToString(), O)) {
			UE_LOG(LogJson, Log, TEXT("Something went wrong here!!"))
		}

		Object = Cast<T>(O);
	}

	Object = Cast<T>(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").TryLoad());
}

bool IImporter::HandleAssetCreation(UObject* Asset)
{
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

TArray<TSharedPtr<FJsonValue>> IImporter::FilterExportsByOuter(const FString& Outer)
{
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

TSharedPtr<FJsonValue> IImporter::GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object)
{
	const TSharedPtr<FJsonObject> ValueObject = TSharedPtr(Object);

	FString StringIndex;
	ValueObject->GetStringField("ObjectPath").Split(".", nullptr, &StringIndex);

	return AllJsonObjects[FCString::Atod(*StringIndex)];
}
