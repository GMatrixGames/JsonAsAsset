// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Importer.h"

class UTextureImporters : public IImporter {
public:
	UTextureImporters(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
	}

	virtual bool ImportData() override;

	// Public as we don't import 2D textures locally at the moment
	bool ImportTexture2D(UTexture*& OutTexture2D, const TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const;
	bool ImportRenderTarget2D(UTexture*& OutRenderTarget2D, const TSharedPtr<FJsonObject>& Properties) const;
};
