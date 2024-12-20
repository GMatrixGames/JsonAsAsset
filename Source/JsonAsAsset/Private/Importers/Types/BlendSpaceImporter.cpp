// Copyright JAA Contributors 2024-2025

#include "Importers/Types/BlendSpaceImporter.h"
#include "Utilities/MathUtilities.h"

void UBlendSpaceDerived::AddSampleOnly(UAnimSequence* AnimationSequence, const FVector& SampleValue) {
	SampleData.Add(FBlendSample(AnimationSequence, SampleValue, true, true));
}

void UBlendSpaceDerived::SetAxisToScaleAnimationInput(const EBlendSpaceAxis AxisToScaleAnimationInput) {
	this->AxisToScaleAnimation = AxisToScaleAnimationInput;
}

void UBlendSpaceDerived::SetBlendParameterPrimary(const FBlendParameter& BlendParametersInput) {
	this->BlendParameters[0] = BlendParametersInput;
}

void UBlendSpaceDerived::SetBlendParameterSecondary(const FBlendParameter& BlendParametersInput) {
	this->BlendParameters[1] = BlendParametersInput;
}

void UBlendSpaceDerived::SetInterpolationParamPrimary(const FInterpolationParameter InterpolationParamInput) {
	this->InterpolationParam[0] = InterpolationParamInput;
}

void UBlendSpaceDerived::SetInterpolationParamSecondary(const FInterpolationParameter InterpolationParamInput) {
	this->InterpolationParam[1] = InterpolationParamInput;
}

void UBlendSpaceDerived::SetNotifyTriggerMode(const ENotifyTriggerMode::Type NotifyTriggerModeInput) {
	this->NotifyTriggerMode = NotifyTriggerModeInput;
}

bool UBlendSpaceImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> AssetData = JsonObject->GetObjectField("Properties");
		UBlendSpace* BlendSpace = NewObject<UBlendSpace>(Package, UBlendSpace::StaticClass(), *FileName, RF_Public | RF_Standalone);
		
		auto SampleData = AssetData->GetArrayField("SampleData");

		BlendSpace->Modify();

		if (AssetData->HasField("BlendParameters")) {
			const TSharedPtr<FJsonObject> BlendParamsObject = AssetData->GetObjectField("BlendParameters");

			FBlendParameter PrimaryBlendParam;
			PrimaryBlendParam.DisplayName = BlendParamsObject->GetStringField("DisplayName");
			PrimaryBlendParam.Min = BlendParamsObject->GetNumberField("Min");
			PrimaryBlendParam.Max = BlendParamsObject->GetNumberField("Max");
			if (BlendParamsObject->HasField("GridNum")) {
				PrimaryBlendParam.GridNum = BlendParamsObject->GetNumberField("GridNum");
			} else {
				PrimaryBlendParam.GridNum = 4;
			}

			Cast<UBlendSpaceDerived>(BlendSpace)->SetBlendParameterPrimary(PrimaryBlendParam);
		}

		if (AssetData->HasField("BlendParameters[1]"))
		{
			const TSharedPtr<FJsonObject> BlendParamsObjectSecondary = AssetData->GetObjectField("BlendParameters[1]");

			FBlendParameter SecondaryBlendParam;
			SecondaryBlendParam.DisplayName = BlendParamsObjectSecondary->GetStringField("DisplayName");
			SecondaryBlendParam.Min = BlendParamsObjectSecondary->GetNumberField("Min");
			SecondaryBlendParam.Max = BlendParamsObjectSecondary->GetNumberField("Max");

			if (BlendParamsObjectSecondary->HasField("GridNum")) {
				SecondaryBlendParam.GridNum = BlendParamsObjectSecondary->GetNumberField("GridNum");
			} else {
				SecondaryBlendParam.GridNum = 4;
			}

			Cast<UBlendSpaceDerived>(BlendSpace)->SetBlendParameterSecondary(SecondaryBlendParam);
		}

		if (AssetData->HasField("InterpolationParam")) {
			const TSharedPtr<FJsonObject> InterpolationParamObject = AssetData->GetObjectField("InterpolationParam");

			FInterpolationParameter PrimaryInterpolationParam;
			PrimaryInterpolationParam.InterpolationTime = InterpolationParamObject->GetNumberField("InterpolationTime");

			Cast<UBlendSpaceDerived>(BlendSpace)->SetInterpolationParamPrimary(PrimaryInterpolationParam);
		}

		if (AssetData->HasField("InterpolationParam[1]")) {
			const TSharedPtr<FJsonObject> InterpolationParamObjectSecondary = AssetData->GetObjectField("InterpolationParam[1]");

			FInterpolationParameter SecondaryInterpolationParam;
			SecondaryInterpolationParam.InterpolationTime = InterpolationParamObjectSecondary->GetNumberField("InterpolationTime");

			Cast<UBlendSpaceDerived>(BlendSpace)->SetInterpolationParamSecondary(SecondaryInterpolationParam);
		}

		for (const TSharedPtr<FJsonValue>& JsonObjectValue : SampleData) {
			const TSharedPtr<FJsonObject> JsonObjectVal = JsonObjectValue->AsObject();

			if (JsonObjectVal.IsValid()) {
				auto AnimationJsonObject = JsonObjectVal->GetObjectField("Animation");

				UObject* Object = StaticLoadObject(UObject::StaticClass(), nullptr, *AnimationJsonObject->GetStringField("PathToObject"));

				BlendSpace->Modify();
				Cast<UBlendSpaceDerived>(BlendSpace)->AddSampleOnly(Cast<UAnimSequence>(Object), FMathUtilities::ObjectToVector(JsonObjectVal->GetObjectField("SampleValue").Get()));
				BlendSpace->PostEditChange();
			}
		}

		auto SerializerProperties = TSharedPtr<FJsonObject>(AssetData);

		TArray<FString> FieldsToRemove = {
			"SampleData",
			"GridSamples",
			"InterpolationParam",
			"InterpolationParam[1]",
			"BlendParameters",
			"BlendParameters[1]",
		};

		for (const FString& FieldName : FieldsToRemove) {
			if (SerializerProperties->HasField(FieldName)) {
				SerializerProperties->RemoveField(FieldName);
			}
		}

		GetObjectSerializer()->DeserializeObjectProperties(AssetData, BlendSpace);

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(BlendSpace)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}