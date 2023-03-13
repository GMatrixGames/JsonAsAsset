// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/Importer.h"

template <typename T>
T* IImporter::LoadObject(const TSharedPtr<FJsonObject>* PackageIndex) {
	FString Type;
	FString Name;
	PackageIndex->Get()->GetStringField("ObjectName").Split("'", &Type, &Name);
	FString Path;
	PackageIndex->Get()->GetStringField("ObjectPath").Split(".", &Path, nullptr);
	return Cast<T>(FSoftObjectPath(Type + "'" + Path + "." + Name + "'").TryLoad());
}


FName IImporter::GetExportNameOfSubobject(const FString& PackageIndex) {
	FString Name;
	PackageIndex.Split("'", nullptr, &Name);
	Name.Split(":", nullptr, &Name);
	Name = Name.Replace(TEXT("'"), TEXT(""));
	return FName(Name);
}
