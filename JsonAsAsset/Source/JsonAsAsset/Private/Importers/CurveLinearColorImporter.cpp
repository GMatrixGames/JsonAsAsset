// Copyright JAA Contributors 2023-2024

#include "Importers/CurveLinearColorImporter.h"

#include "JsonGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveLinearColor.h"
#include "Dom/JsonObject.h"
#include "Factories/CurveFactory.h"
#include "Utilities/MathUtilities.h"

#include "UObject/SavePackage.h"

bool UCurveLinearColorImporter::ImportData() {
	try {
		// Array of containers
		TArray<TSharedPtr<FJsonValue>> FloatCurves = JsonObject->GetArrayField("FloatCurves");

		UCurveLinearColorFactory* CurveFactory = NewObject<UCurveLinearColorFactory>();
		UCurveLinearColor* LinearCurveAsset = Cast<UCurveLinearColor>(CurveFactory->FactoryCreateNew(UCurveLinearColor::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		// for each container, get keys
		for (int i = 0; i < FloatCurves.Num(); i++) {
			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurves[i]->AsObject()->GetArrayField("Keys");
			LinearCurveAsset->FloatCurves[i].Keys.Empty();

			// add keys to array
			for (int j = 0; j < Keys.Num(); j++) {
				LinearCurveAsset->FloatCurves[i].Keys.Add(FMathUtilities::ObjectToRichCurveKey(Keys[j]->AsObject()));
			}
		}

		SavePackage();
		if (!HandleAssetCreation(LinearCurveAsset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}