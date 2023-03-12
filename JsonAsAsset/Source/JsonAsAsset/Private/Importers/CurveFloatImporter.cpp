// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/CurveFloatImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Factories/CurveFactory.h"

bool UCurveFloatImporter::ImportData() {
	try {
		TArray<TSharedPtr<FJsonValue>> Keys = JsonObject->GetObjectField("Properties")->GetObjectField("FloatCurve")->GetArrayField("Keys");

		UCurveFloatFactory* CurveFactory = NewObject<UCurveFloatFactory>();
		UCurveFloat* CurveAsset = Cast<UCurveFloat>(CurveFactory->FactoryCreateNew(UCurveFloat::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		FAssetRegistryModule::AssetCreated(CurveAsset);
		if (!CurveAsset->MarkPackageDirty()) return false;
		Package->SetDirtyFlag(true);
		CurveAsset->PostEditChange();
		CurveAsset->AddToRoot();

		for (int32 key_index = 0; key_index < Keys.Num(); key_index++) {
			const TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();

			ERichCurveInterpMode InterpMode;
			FString StringInterpMode = Key->GetStringField("InterpMode");

			if (StringInterpMode.EndsWith("RCIM_Linear")) InterpMode = RCIM_Linear;
			else if (StringInterpMode.EndsWith("RCIM_Cubic")) InterpMode = RCIM_Cubic;
			else if (StringInterpMode.EndsWith("RCIM_Constant")) InterpMode = RCIM_Constant;
			else InterpMode = RCIM_None;

			FRichCurveKey RichKey = FRichCurveKey(Key->GetNumberField("Time"), Key->GetNumberField("Value"), Key->GetNumberField("ArriveTangent"),
			                                      Key->GetNumberField("LeaveTangent"), InterpMode);

			CurveAsset->FloatCurve.Keys.Add(RichKey);
		}
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
