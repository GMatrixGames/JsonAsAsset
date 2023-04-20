// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "JsonAsAssetStyle.h"

class FJsonAsAssetCommands : public TCommands<FJsonAsAssetCommands> {
	public:
		FJsonAsAssetCommands()
			: TCommands(TEXT("JsonAsAsset"), NSLOCTEXT("Contexts", "JsonAsAsset", "JsonAsAsset Plugin"), NAME_None, FJsonAsAssetStyle::GetStyleSetName()) {
		}

		// TCommands<> interface
		virtual void RegisterCommands() override;

		TSharedPtr<FUICommandInfo> PluginAction;
};
