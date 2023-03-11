// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct ImportOptionsBase {
	bool bImportWithPath;
};

class FToolBarBuilder;
class FMenuBuilder;

class FJsonAsAssetModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
private:
	void RegisterMenus();
private:
	TSharedPtr<class FUICommandList> PluginCommands;
	FVector ObjectToVector(FJsonObject* Object);
	FRotator ObjectToRotator(FJsonObject* Object);
	FQuat ObjectToQuat(FJsonObject* Object);
	FLinearColor ObjectToLinearColor(FJsonObject* Object);

	/*  
	* Function to call normal executions for animation sequences
	* to add curves, notifies and other needed things.
	*/
	bool EvaluateAnimSequence(FJsonObject* Object, UAnimSequenceBase* AnimSequenceBase);

	/*
	* Get selected assets in the content browser of the use,
	* usually used for the plugin, editing the existing assets.
	*/
	UObject* GetSelectedAsset();

	UPackage* CreateAssetPackage(FString Name, TArray<FString> Files);

};
