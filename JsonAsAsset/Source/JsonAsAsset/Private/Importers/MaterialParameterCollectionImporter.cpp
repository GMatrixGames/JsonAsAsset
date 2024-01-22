// Copyright JAA Contributors 2023-2024

#include "Importers/MaterialParameterCollectionImporter.h"

#include "UObject/SavePackage.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialParameterCollection.h"
#include "Utilities/MathUtilities.h"

bool UMaterialParameterCollectionImporter::ImportData() {
	try {
		// Query properties for multi-use purposes
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		UMaterialParameterCollection* MaterialParameterCollection = NewObject<UMaterialParameterCollection>(Package, UMaterialParameterCollection::StaticClass(), *FileName, RF_Public | RF_Standalone);
		MaterialParameterCollection->StateId = FGuid(Properties->GetStringField("StateId"));

		if (const TArray<TSharedPtr<FJsonValue>>* ScalarParametersPtr; Properties->TryGetArrayField("ScalarParameters", ScalarParametersPtr)) {
			for (const TSharedPtr<FJsonValue> ScalarParameter : *ScalarParametersPtr) {
				TSharedPtr<FJsonObject> _ScalarParameter = ScalarParameter->AsObject();
				FCollectionScalarParameter ScalarParameter_Collection = FCollectionScalarParameter();

				ScalarParameter_Collection.DefaultValue = _ScalarParameter->GetNumberField("DefaultValue");
				ScalarParameter_Collection.ParameterName = FName(_ScalarParameter->GetStringField("ParameterName"));
				ScalarParameter_Collection.Id = FGuid(_ScalarParameter->GetStringField("ID"));

				MaterialParameterCollection->ScalarParameters.Add(ScalarParameter_Collection);
			}
		}

		if (const TArray<TSharedPtr<FJsonValue>>* VectorParametersPtr; Properties->TryGetArrayField("VectorParameters", VectorParametersPtr)) {
			for (const TSharedPtr<FJsonValue> VectorParameter : *VectorParametersPtr) {
				TSharedPtr<FJsonObject> _VectorParameter = VectorParameter->AsObject();
				FCollectionVectorParameter VectorParameter_Collection = FCollectionVectorParameter();

				VectorParameter_Collection.DefaultValue = FMathUtilities::ObjectToLinearColor(_VectorParameter->GetObjectField("DefaultValue").Get());
				VectorParameter_Collection.ParameterName = FName(_VectorParameter->GetStringField("ParameterName"));
				VectorParameter_Collection.Id = FGuid(_VectorParameter->GetStringField("ID"));

				MaterialParameterCollection->VectorParameters.Add(VectorParameter_Collection);
			}
		}

		SavePackage();
		if (!HandleAssetCreation(MaterialParameterCollection)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
