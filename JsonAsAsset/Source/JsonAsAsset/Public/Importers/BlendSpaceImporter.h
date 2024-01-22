// Copyright JAA Contributors 2023-2024

#pragma once

#include "Importer.h"
#include "Animation/BlendSpace.h"

// We use this to set variables
// in the blend space asset
class UBlendSpaceDerived : public UBlendSpace {
public:
	void SetAxisToScaleAnimationInput(const EBlendSpaceAxis AxisToScaleAnimationInput);
	void SetBlendParameterPrimary(const FBlendParameter& BlendParametersInput);
	void SetBlendParameterSecondary(const FBlendParameter& BlendParametersInput);
	void SetInterpolationParamPrimary(const FInterpolationParameter InterpolationParamInput);
	void SetInterpolationParamSecondary(const FInterpolationParameter InterpolationParamInput);
	void SetNotifyTriggerMode(const ENotifyTriggerMode::Type NotifyTriggerModeInput);
};

class UBlendSpaceImporter : public IImporter {
public:
	UBlendSpaceImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg) {
	}

	virtual bool ImportData() override;
};