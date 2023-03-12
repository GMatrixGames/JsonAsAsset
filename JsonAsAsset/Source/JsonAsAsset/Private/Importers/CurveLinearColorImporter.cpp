// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/CurveLinearColorImporter.h"

#include "JsonGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveLinearColor.h"
#include "Dom/JsonObject.h"
#include "Factories/CurveFactory.h"

bool UCurveLinearColorImporter::ImportData()
{
	try {
		TArray<TSharedPtr<FJsonValue>> FloatCurves = JsonObject->GetArrayField("FloatCurves");

		UCurveLinearColorFactory* CurveFactory = NewObject<UCurveLinearColorFactory>();
		UCurveLinearColor* LinearCurveAsset = Cast<UCurveLinearColor>(
			CurveFactory->FactoryCreateNew(UCurveLinearColor::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		FAssetRegistryModule::AssetCreated(LinearCurveAsset);
		if (!LinearCurveAsset->MarkPackageDirty()) return false;
		Package->SetDirtyFlag(true);
		LinearCurveAsset->PostEditChange();
		LinearCurveAsset->AddToRoot();

		for (int i = 0; i < FloatCurves.Num(); i++)
		{
			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurves[i]->AsObject()->GetArrayField("Keys");

			LinearCurveAsset->FloatCurves[i].Keys.Empty();

			for (int j = 0; j < Keys.Num(); j++)
			{
				FRichCurve RichCurveObject = FRichCurve();

				const TSharedPtr<FJsonObject> Key = Keys[j]->AsObject();
				ERichCurveInterpMode InterpMode;

				FString StringInterpMode = Key->GetStringField("InterpMode");

				if (StringInterpMode == "RCIM_Linear") InterpMode = RCIM_Linear;
				else if (StringInterpMode == "RCIM_Cubic") InterpMode = RCIM_Cubic;
				else if (StringInterpMode == "RCIM_Constant") InterpMode = RCIM_Constant;
				else InterpMode = RCIM_None;

				FRichCurveKey RichKey = FRichCurveKey(Key->GetNumberField("Time"), Key->GetNumberField("Value"), Key->GetNumberField("ArriveTangent"),
				                                      Key->GetNumberField("LeaveTangent"), InterpMode);

				LinearCurveAsset->FloatCurves[i].Keys.Add(RichKey);
			}
		}
	}
	catch (const char* Exception)
	{
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
