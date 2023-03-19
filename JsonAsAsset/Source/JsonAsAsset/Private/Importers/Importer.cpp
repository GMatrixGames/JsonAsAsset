// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Importer.h"

#include "AssetRegistry/AssetRegistryModule.h"

template <typename T>
T* IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
	Name = Name.Replace(TEXT("'"), TEXT(""));
	return Cast<T>(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").TryLoad());
}

bool IImporter::HandleAssetCreation(UObject* Asset)
{
	FAssetRegistryModule::AssetCreated(Asset);
	if (!Asset->MarkPackageDirty()) return false;
	Package->SetDirtyFlag(true);
	Asset->PostEditChange();
	Asset->AddToRoot();

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
		TSharedPtr<FJsonObject> ValueObject = TSharedPtr<FJsonObject>(Value->AsObject());

		if (ValueObject->GetStringField("Outer") == Outer) {
			ReturnValue.Add(Value);
		}
	}

	return AllJsonObjects;
}
