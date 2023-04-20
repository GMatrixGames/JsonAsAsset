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
	// FModel Exports Folder
	UPROPERTY(config, EditAnywhere, Category = "Asset | Configuration")
	FDirectoryPath ExportDirectory;

	// Automate importing references
	UPROPERTY(config, EditAnywhere, Category = "Asset | Configuration")
	bool bAutomateReferences;

	UPROPERTY(config, EditAnywhere, Category = "Asset")
	bool bTextureRemoteDownload;
};