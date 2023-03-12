// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/ParticleModuleTypeDataBeam2Importer.h"

#include "Distributions.h"

// Testing for particles
bool UParticleModuleTypeDataBeam2Importer::ImportData() {
	try {
		const TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> TaperScale = Properties->GetObjectField("TaperScale");
		const TSharedPtr<FJsonObject> TableObject = TaperScale->GetObjectField("Table");

		FDistributionLookupTable LookupTable = FDistributionLookupTable();

		LookupTable.Op = TableObject->GetIntegerField("Op");
		LookupTable.EntryCount = TableObject->GetIntegerField("EntryCount");
		LookupTable.EntryStride = TableObject->GetIntegerField("EntryStride");
		LookupTable.TimeScale = TableObject->GetNumberField("TimeScale");
		LookupTable.SubEntryStride = TableObject->GetNumberField("SubEntryStride");
		LookupTable.LockFlag = TableObject->GetNumberField("LockFlag");

		// Float Array
		TArray<TSharedPtr<FJsonValue>> Values = TableObject->GetArrayField("Values");

		for (int i = 0; i < Values.Num(); i++) {
			LookupTable.Values.Add(Values[i]->AsNumber());
		}

		GLog->Log("JsonAsAsset: Lookup Table GetValuesPerEntry: " + FString(*FString::SanitizeFloat(LookupTable.GetValuesPerEntry())));
		GLog->Log("JsonAsAsset: Lookup Table GetValueCount: " + FString(*FString::SanitizeFloat(LookupTable.GetValueCount())));

		FString UEA = FString("(");

		for (int x = 0; x < LookupTable.GetValueCount() * 2; x += 1) {
			float Time = x / 1000.0f;
			float OutValue = 0.1;

			const float* Entry1;
			const float* Entry2;
			float Alpha;

			LookupTable.GetEntry(Time, Entry1, Entry2, Alpha);
			OutValue = FMath::Lerp(Entry1[0], Entry2[0], Alpha);

			GLog->Log("JsonAsAsset: Value: " + FString(*FString::SanitizeFloat(OutValue)) + "Time: " + FString(*FString::SanitizeFloat(Time)));

			UEA = UEA + FString("(InVal=" + FString(*FString::SanitizeFloat(Time)) + ",OutVal=" + FString(*FString::SanitizeFloat(OutValue)) + "),");
		}

		UEA += ')';

		GLog->Log(UEA);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
