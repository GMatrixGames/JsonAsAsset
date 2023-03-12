// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/CurveFloatImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Factories/CurveFactory.h"
#include "Utilities/AssetUtilities.h"

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

		for (int32 i = 0; i < Keys.Num(); i++) {
			CurveAsset->FloatCurve.Keys.Add(FAssetUtilities::ObjectToRichCurveKey(Keys[i]->AsObject()));
		}
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
