// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetUtilities
{
public:
	static UPackage* CreateAssetPackage(const FString& FullPath);
	static UPackage* CreateAssetPackage(const FString& Name, const FString& OutputPath);
	static UPackage* CreateAssetPackage(const FString& Name, const FString& OutputPath, UPackage*& OutOutermostPkg);
	
	/**
	 * Get the asset currently selected in the Content Browser.
	 * 
	 * @return Selected Asset
	 */
	static UObject* GetSelectedAsset();

	static FRichCurveKey ObjectToRichCurveKey(const TSharedPtr<FJsonObject>& Object);
};
