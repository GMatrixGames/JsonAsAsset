#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "JsonAsAssetSettings.generated.h"

// A editor plugin to allow JSON files from FModel to a asset in the Content Browser
UCLASS(config = Engine, defaultconfig)
class UJsonAsAssetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	public:
		UJsonAsAssetSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif
	// Browse to newly added Asset
	UPROPERTY(config, EditAnywhere, Category = Behavior)
	bool bJumpToAsset;

	UPROPERTY(config, EditAnywhere, Category = Textures)
	bool bTextureRemoteDownload;
};

// Taken from: https://github.com/nachomonkey/RefreshAllNodes