// Copyright JAA Contributors 2023-2024

#include "Importers/CurveFloatImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Factories/CurveFactory.h"
#include "Utilities/MathUtilities.h"

bool UCurveFloatImporter::ImportData() {
	try {
		// Quick way to access the curve keys
		TArray<TSharedPtr<FJsonValue>> Keys = JsonObject->GetObjectField("Properties")->GetObjectField("FloatCurve")->GetArrayField("Keys");

		UCurveFloatFactory* CurveFactory = NewObject<UCurveFloatFactory>();
		UCurveFloat* CurveAsset = Cast<UCurveFloat>(CurveFactory->FactoryCreateNew(UCurveFloat::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		// Add Rich Keys
		for (TSharedPtr<FJsonValue>& Key : Keys)
			CurveAsset->FloatCurve.Keys.Add(FMathUtilities::ObjectToRichCurveKey(Key->AsObject()));

		// Handle edit changes, and add it to the content browser
		SavePackage();
		if (!HandleAssetCreation(CurveAsset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}