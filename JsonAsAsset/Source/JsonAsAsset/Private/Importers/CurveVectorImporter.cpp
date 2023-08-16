// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/CurveVectorImporter.h"

#include "Dom/JsonObject.h"
#include "Factories/CurveFactory.h"
#include "Curves/CurveVector.h"

#include "Utilities/MathUtilities.h"

bool UCurveVectorImporter::ImportData() {
	try {
		// Array of containers
		TArray<TSharedPtr<FJsonValue>> FloatCurves = JsonObject->GetArrayField("FloatCurves");

		UCurveVectorFactory* CurveVectorFactory = NewObject<UCurveVectorFactory>();
		UCurveVector* CurveVectorAsset = Cast<UCurveVector>(CurveVectorFactory->FactoryCreateNew(UCurveVector::StaticClass(), Cast<UObject>(OutermostPkg), *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		// for each container, get keys
		for (int i = 0; i < FloatCurves.Num(); i++) {
			TArray<TSharedPtr<FJsonValue>> Keys = FloatCurves[i]->AsObject()->GetArrayField("Keys");
			CurveVectorAsset->FloatCurves[i].Keys.Empty();

			// add keys to array
			for (int j = 0; j < Keys.Num(); j++) {
				CurveVectorAsset->FloatCurves[i].Keys.Add(FMathUtilities::ObjectToRichCurveKey(Keys[j]->AsObject()));
			}
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(CurveVectorAsset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}