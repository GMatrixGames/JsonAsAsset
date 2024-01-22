// Copyright JAA Contributors 2023-2024

#pragma once

class FAssetUtilities {
public:
	/*
	* Creates a UPackage to create assets in the Content Browser.
	* 
	* @return Package
	*/
	static UPackage* CreateAssetPackage(const FString& FullPath);
	static UPackage* CreateAssetPackage(const FString& Name, const FString& OutputPath);
	static UPackage* CreateAssetPackage(const FString& Name, const FString& OutputPath, UPackage*& OutOutermostPkg);
	
	/**
	 * Get the asset currently selected in the Content Browser.
	 * 
	 * @return Selected Asset
	 */
	static UObject* GetSelectedAsset();

	// Purpose: Wrapping references before they get set
	//          to import them
public:
	template <class T = UObject>
	static bool ConstructAsset(const FString& Path, const FString& Type, TObjectPtr<T>& OutObject, bool& bSuccess);
	static bool Construct_TypeTexture(const FString& Path, const FString& RealPath, UTexture*& OutTexture);

	static void CreatePlugin(FString PluginName);

	static const TSharedPtr<FJsonObject> API_RequestExports(const FString& Path);
};
