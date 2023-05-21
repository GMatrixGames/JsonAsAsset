#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "IDetailCustomization.h"
#include "JsonAsAssetSettings.generated.h"

// Grabbed from (https://github.com/FabianFG/CUE4Parse/blob/master/CUE4Parse/UE4/Versions/EGame.cs)
UENUM()
enum EParseVersion {
	GAME_UE4_0,
	GAME_UE4_1,
	GAME_UE4_2,
	GAME_UE4_3,
	GAME_UE4_4,
	GAME_UE4_5,
	GAME_ArkSurvivalEvolved,
	GAME_UE4_6,
	GAME_UE4_7,
	GAME_UE4_8,
	GAME_UE4_9,
	GAME_UE4_10,
	GAME_SeaOfThieves,
	GAME_UE4_11,
	GAME_GearsOfWar4,
	GAME_UE4_12,
	GAME_UE4_13,
	GAME_StateOfDecay2,
	GAME_UE4_14,
	GAME_TEKKEN7,
	GAME_UE4_15,
	GAME_UE4_16,
	GAME_PlayerUnknownsBattlegrounds,
	GAME_TrainSimWorld2020,
	GAME_UE4_17,
	GAME_LifeIsStrange2,
	GAME_AWayOut,
	GAME_UE4_18,
	GAME_KingdomHearts3,
	GAME_FinalFantasy7Remake,
	GAME_AceCombat7,
	GAME_UE4_19,
	GAME_Paragon,
	GAME_UE4_20,
	GAME_Borderlands3,
	GAME_UE4_21,
	GAME_StarWarsJediFallenOrder,
	GAME_UE4_22,
	GAME_UE4_23,
	GAME_ApexLegendsMobile,
	GAME_UE4_24,
	GAME_UE4_25,
	GAME_RogueCompany,
	GAME_DeadIsland2,
	GAME_KenaBridgeofSpirits,
	GAME_UE4_25_Plus,
	GAME_UE4_26,
	GAME_GTATheTrilogyDefinitiveEdition,
	GAME_ReadyOrNot,
	GAME_Valorant,
	GAME_TowerOfFantasy,
	GAME_Dauntless,
	GAME_TheDivisionResurgence,
	GAME_UE4_27,
	GAME_Splitgate,
	GAME_HYENAS,
	GAME_HogwartsLegacy,
	GAME_UE4_28,

	GAME_UE4_LATEST,

	GAME_UE5_0,
	GAME_MeetYourMaker,
	GAME_UE5_1,
	GAME_UE5_2,
	GAME_UE5_3,

	GAME_UE5_LATEST
};

USTRUCT()
struct FParseKey {
	GENERATED_BODY()

	FParseKey() {
	}

	FParseKey(FString NewGUID, FString NewKey) {
		Value = NewKey;
		Guid = NewGUID;
	}

	TArray<uint8> Key;

	// Must start with 0x
	UPROPERTY(Config, EditAnywhere, Category = "Key")
	FString Value;

	UPROPERTY(Config, EditAnywhere, Category = "Key")
	FString Guid;
};

// Buttons in plugin settings
class FJsonAsAssetSettingsDetails : public IDetailCustomization {
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

// A editor plugin to allow JSON files from FModel to a asset in the Content Browser
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class UJsonAsAssetSettings : public UDeveloperSettings {
	GENERATED_BODY()

public:
	UJsonAsAssetSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif

	// FModel Exports Folder
	// NOTE: Please use the file selector, do not manually paste it 
	UPROPERTY(Config, EditAnywhere, Category="Asset|Configuration")
	FDirectoryPath ExportDirectory;

	// Saves packages when imported
	UPROPERTY(Config, EditAnywhere, Category="Asset|Configuration")
	bool bSavePackages;

	// Fixes errors with textures by not connecting nodes
	UPROPERTY(Config, EditAnywhere, Category="Asset|Configuration")
	bool bExposePins;

	// Fetches assets from a local service and automatically
	// imports them into your project, without having them locally
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch")
	bool bEnableLocalFetch;

	// Pak files location
	// NOTE: Please use the file selector, do not manually paste it 
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch|Configuration", meta=(EditCondition="bEnableLocalFetch"))
	FDirectoryPath ArchiveDirectory;

	// UE Version for the Unreal Engine Game (same as FModel's UE Verisons property)
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch|Configuration", meta=(EditCondition="bEnableLocalFetch", GetOptions="GetParseVersions"))
	FString UnrealVersion;

	UFUNCTION(CallInEditor)
	TArray<FString> GetParseVersions();

	// Mappings file
	// NOTE: Please use the file selector, do not manually paste it 
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch|Configuration", meta=(EditCondition="bEnableLocalFetch", FilePathFilter="usmap", RelativeToGameDir))
	FFilePath MappingFilePath;

	// High res textures
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch|Configuration", meta=(EditCondition="bEnableLocalFetch"))
	bool bUseContentBuilds;

	// Main key for archives
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch Encryption", meta=(EditCondition="bEnableLocalFetch", DisplayName="Archive Key"))
	FString ArchiveKey;

	// AES Keys
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch Encryption", meta=(EditCondition="bEnableLocalFetch", DisplayName="Dynamic Keys"))
	TArray<FParseKey> DynamicKeys;

	UPROPERTY(Config, EditAnywhere, Category = "Local Fetch Director", meta = (EditConditionHides, EditCondition = "bEnableLocalFetch"))
	bool bHideConsole;

	// Enables the option to change the api's URL
	UPROPERTY(Config, EditAnywhere, Category = "Local Fetch Director", meta = (EditConditionHides, EditCondition = "bEnableLocalFetch"))
	bool bChangeURL;

	// "localhost" is default
	UPROPERTY(Config, EditAnywhere, Category="Local Fetch Director|REST API", meta=(EditConditionHides, EditCondition="bChangeURL && bEnableLocalFetch", DisplayName = "Local URL"))
	FString Url = "http://localhost:1500";
};
