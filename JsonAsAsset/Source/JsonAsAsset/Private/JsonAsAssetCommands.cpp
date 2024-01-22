// Copyright JAA Contributors 2023-2024

#include "JsonAsAssetCommands.h"

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

void FJsonAsAssetCommands::RegisterCommands() {
	UI_COMMAND(PluginAction, "JsonAsAsset", "Choose a JSON file to Convert", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
