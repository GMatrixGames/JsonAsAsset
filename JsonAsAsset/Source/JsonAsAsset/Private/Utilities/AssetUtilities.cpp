// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/AssetUtilities.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Dom/JsonObject.h"

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

FRichCurveKey FAssetUtilities::ObjectToRichCurveKey(TSharedPtr<FJsonObject> Object) {
	ERichCurveInterpMode InterpMode;
	FString StringInterpMode = Object->GetStringField("InterpMode");

	if (StringInterpMode == "RCIM_Linear") InterpMode = RCIM_Linear;
	else if (StringInterpMode == "RCIM_Cubic") InterpMode = RCIM_Cubic;
	else if (StringInterpMode == "RCIM_Constant") InterpMode = RCIM_Constant;
	else InterpMode = RCIM_None;

	return FRichCurveKey(Object->GetNumberField("Time"), Object->GetNumberField("Value"), Object->GetNumberField("ArriveTangent"), Object->GetNumberField("LeaveTangent"), InterpMode);
}
