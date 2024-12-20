// Copyright JAA Contributors 2024-2025

#pragma once

#include "./Importer.h"
#include "Interfaces/IHttpRequest.h"
#include "Sound/SoundNodeModulatorContinuous.h"
#include "Sound/SoundNodeWavePlayer.h"

// Sound Graph Handler
// Handles everything needed to create a sound graph from JSON.
class ISoundGraph : public IImporter {
public:
	ISoundGraph(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IImporter(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;

	// Graph Functions
	static void ConnectEdGraphNode(UEdGraphNode* NodeToConnect, UEdGraphNode* NodeToConnectTo, int Pin);
	static void ConnectSoundNode(USoundNode* NodeToConnect, USoundNode* NodeToConnectTo, int Pin);

	// Creates a empty USoundNode
	static USoundNode* CreateEmptyNode(FName Name, FName Type, USoundCue* SoundCue);
	
	void ConstructNodes(USoundCue* SoundCue, TArray<TSharedPtr<FJsonValue>> JsonArray, TMap<FString, USoundNode*>& OutNodes);
	void SetupNodes(USoundCue* SoundCueAsset, TMap<FString, USoundNode*> SoundCueNodes, TArray<TSharedPtr<FJsonValue>> JsonObjectArray);

	// Sound Wave Import
	void ImportSoundWave(FString URL, FString SavePath, FString AssetPtr, USoundNodeWavePlayer* Node);
	void OnDownloadSoundWave(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString SavePath, FString AssetPtr, USoundNodeWavePlayer* Node);
};
