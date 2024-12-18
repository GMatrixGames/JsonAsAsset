// Copyright JAA Contributors 2024-2025

#pragma once

#include "./Details/JsonAsAssetSettingsDetails.h"

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "JsonAsAssetSettings.generated.h"

// Grabbed from (https://github.com/FabianFG/CUE4Parse/blob/master/CUE4Parse/UE4/Versions/EGame.cs)
UENUM()
enum EParseVersion
{
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
	GAME_UE4_25_Plus,
	GAME_RogueCompany,
	GAME_DeadIsland2,
	GAME_KenaBridgeofSpirits,
	GAME_CalabiYau,
	GAME_UE4_26,
	GAME_GTATheTrilogyDefinitiveEdition,
	GAME_ReadyOrNot,
	GAME_BladeAndSoul,
	GAME_TowerOfFantasy,
	GAME_Dauntless,
	GAME_TheDivisionResurgence,
	GAME_StarWarsJediSurvivor,
	GAME_Snowbreak,
	GAME_UE4_27,
	GAME_Splitgate,
	GAME_HYENAS,
	GAME_HogwartsLegacy,
	GAME_OutlastTrials,
	GAME_Valorant,
	GAME_Gollum,
	GAME_Grounded,
	GAME_UE4_28,

	GAME_UE5_0,
	GAME_MeetYourMaker,
	GAME_UE5_1,
	GAME_UE5_2,
	GAME_UE5_3,
	GAME_UE5_4,
	GAME_UE5_5,
	GAME_UE5_6,

	// Change this always to the last available enum above
	GAME_UE5_LATEST = GAME_UE5_6
};

USTRUCT()
struct FParseKey
{
	GENERATED_BODY()

	FParseKey()
	{
	}

	FParseKey(FString NewGUID, FString NewKey)
	{
		Value = NewKey;
		Guid = NewGUID;
	}

	TArray<uint8> Key;

	// Must start with 0x
	UPROPERTY(EditAnywhere, Config, Category = "Key")
	FString Value;

	UPROPERTY(EditAnywhere, Config, Category = "Key")
	FString Guid;
};

USTRUCT()
struct FMaterialImportSettings
{
	GENERATED_BODY()

	/**
	* When importing/downloading the asset type Material, a error may occur
	* | Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly
	*
	* To counter that error, we skip connecting the inputs to the main result
	* node in the material. If you do use this, import a material, save everything,
	* restart, and re-import the material without any problems with this turned off/on.
	*
	* (or you could just connect them yourself)
	*/
	UPROPERTY(EditAnywhere, Config)
	bool bSkipResultNodeConnection;
};

USTRUCT()
struct FAssetSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config)
	FMaterialImportSettings MaterialImportSettings;

	UPROPERTY(EditAnywhere, Config, meta = (DisplayName = "Save Assets On Import"))
	bool bSavePackagesOnImport;

	/**
	* Not needed for normal operations, needed for older versions of game builds.
	*/
	UPROPERTY(EditAnywhere, Config)
	FString GameName;
};

// A editor plugin to allow JSON files from FModel to a asset in the Content Browser
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class UJsonAsAssetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UJsonAsAssetSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif

	/**
	 * Directory path for exporting FModel assets.
	 * (Output/Exports)
	 *
	 * NOTE: Use the file selector to choose a directory. 
	 *       Avoid manually entering the path or replacing "\" with "/". 
	 *       The file selector ensures proper formatting.
	*/
	UPROPERTY(EditAnywhere, Config, Category="Configuration")
	FDirectoryPath ExportDirectory;

	UPROPERTY(EditAnywhere, Config, Category = "Configuration")
	FAssetSettings AssetSettings;

	/**
	 * Fetches assets from a local hosted API and imports them directly into your project.
	 * 
	 * NOTE: Ensure all settings are correctly configured before starting Local Fetch. 
	 *       Please refer to the README.md file.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch")
	bool bEnableLocalFetch;

	/**
	 * Location of the Paks folder containing all the assets.
	 * (Content/Paks)
	 * 
	 * NOTE: Use the file selector to choose the folder location. 
	 *       Avoid manually pasting the path or replacing "\" with "/".
	 */
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch - Configuration", meta=(EditCondition="bEnableLocalFetch"))
	FDirectoryPath ArchiveDirectory;

	// UE Version for the Unreal Engine Game (same as FModel's UE Verisons property)
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch - Configuration", meta=(EditCondition="bEnableLocalFetch"))
	TEnumAsByte<EParseVersion> UnrealVersion;

	UFUNCTION(CallInEditor)
	static TArray<FString> GetParseVersions();

	// Path to the mappings file.
	//
	// NOTE: Use the file selector to choose the file location. 
	//       Avoid manually pasting the path.
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch - Configuration", meta=(EditCondition="bEnableLocalFetch", FilePathFilter="usmap", RelativeToGameDir))
	FFilePath MappingFilePath;

	UPROPERTY(EditAnywhere, Config, Category="Local Fetch - Encryption", meta=(EditCondition="bEnableLocalFetch"), AdvancedDisplay)
	bool bDownloadExistingTextures;

	// Main key for archives
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch - Encryption", meta=(EditCondition="bEnableLocalFetch", DisplayName="Archive Key"))
	FString ArchiveKey;

	// AES Keys
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch - Encryption", meta=(EditCondition="bEnableLocalFetch", DisplayName="Dynamic Keys"))
	TArray<FParseKey> DynamicKeys;

	// Enables the option to change the api's URL
	//
	// DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING
	UPROPERTY(EditAnywhere, Config, Category = "Local Fetch", meta = (EditCondition = "bEnableLocalFetch"), AdvancedDisplay)
	bool bChangeURL;

	// "http://localhost:1500" is default
	UPROPERTY(EditAnywhere, Config, Category="Local Fetch", meta=(EditCondition="bChangeURL && bEnableLocalFetch", DisplayName = "Local URL"), AdvancedDisplay)
	FString Url = "http://localhost:1500";
};
