#pragma once

class FRemoteAssetDownloader {
public:
	static bool MakeTexture(const FString& Path, UTexture2D*& OutTexture);
	static bool MakeMaterialParameterCollection(const FString& Path, UMaterialParameterCollection*& OutCollection);
};
