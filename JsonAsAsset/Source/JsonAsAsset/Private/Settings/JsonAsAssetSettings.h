#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "JsonAsAssetSettings.generated.h"

// A editor plugin to allow JSON files from FModel to a asset in the Content Browser
UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig)
class UJsonAsAssetSettings : public UDeveloperSettings {
	GENERATED_BODY()

public:
	UJsonAsAssetSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif

	// FModel Exports Folder
	UPROPERTY(config, EditAnywhere, Category = "Asset|Configuration")
	FDirectoryPath ExportDirectory;

	// Automate importing references
	UPROPERTY(config, EditAnywhere, Category = "Asset|Configuration")
	bool bAutomateReferences;

	// Downloads assets from FortniteCentral and automatically
	// imports them into your project, without having them locally
	// (only supports: Fortnite)
	//
	// NOTE: Please use the file selector, do not manually paste it 
	UPROPERTY(config, EditAnywhere, Category = "Asset")
	bool bEnableRemoteDownload;
};
