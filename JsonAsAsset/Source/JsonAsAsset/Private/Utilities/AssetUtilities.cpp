// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/AssetUtilities.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Dom/JsonObject.h"

UPackage* FAssetUtilities::CreateAssetPackage(const FString& FullPath) {
	UPackage* Package = CreatePackage(*FullPath);
	UPackage* _ = Package->GetOutermost();
	Package->FullyLoad();

	GLog->Log(FullPath);

	return Package;
}

UPackage* FAssetUtilities::CreateAssetPackage(const FString& Name, const FString& OutputPath) {
	UPackage* Ignore = nullptr;
	return CreateAssetPackage(Name, OutputPath, Ignore);
}

UPackage* FAssetUtilities::CreateAssetPackage(const FString& Name, const FString& OutputPath, UPackage*& OutOutermostPkg) {
	// TODO: Support virtual paths (plugins)
	FString Path;

	OutputPath.Split("FortniteGame/Content", nullptr, &Path);
	Path.Split("/", &Path, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	const FString PathWithGame = "/Game" + Path + "/" + Name;
	UPackage* Package = CreatePackage(*PathWithGame);
	OutOutermostPkg = Package->GetOutermost();
	Package->FullyLoad();

	GLog->Log(Path);

	return Package;
}

UObject* FAssetUtilities::GetSelectedAsset() {
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.Num() == 0) {
		GLog->Log("JsonAsAsset: [GetSelectedAsset] None selected, returning nullptr.");

		const FText DialogText = FText::FromString(TEXT("A function to find a selected asset failed, please select a asset to go further."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		// None found, therefore we need to return nullptr.
		return nullptr;
	}

	// Return only the first selected asset
	return SelectedAssets[0].GetAsset();
}

FRichCurveKey FAssetUtilities::ObjectToRichCurveKey(const TSharedPtr<FJsonObject>& Object) {
	ERichCurveInterpMode InterpMode;
	FString StringInterpMode = Object->GetStringField("InterpMode");

	if (StringInterpMode == "RCIM_Linear") InterpMode = RCIM_Linear;
	else if (StringInterpMode == "RCIM_Cubic") InterpMode = RCIM_Cubic;
	else if (StringInterpMode == "RCIM_Constant") InterpMode = RCIM_Constant;
	else InterpMode = RCIM_None;

	return FRichCurveKey(Object->GetNumberField("Time"), Object->GetNumberField("Value"), Object->GetNumberField("ArriveTangent"), Object->GetNumberField("LeaveTangent"), InterpMode);
}

UEnum* FAssetUtilities::GetEnumOfType(const FString& ScriptPath) {
	return FindObject<UEnum>(nullptr, *ScriptPath);
}
