// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/SoundCueImporter.h"
#include "Sound/SoundCue.h"
#include <Runtime/Engine/Classes/Sound/SoundNodeWavePlayer.h>

bool USoundCueImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->SetPackageForDeserialization(Package);

		USoundCue* SoundCue = NewObject<USoundCue>(Package, *FileName, RF_Public | RF_Standalone);
		TArray<TSharedRef<FJsonObject>> NodeTree;
		SoundCue->CreateGraph();
		SoundCue->Modify();

		// Sort, and find each node
		for (const TSharedPtr<FJsonValue> Export : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = Export->AsObject();
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Object->GetStringField("Type"));

			if (Object->GetStringField("Type").StartsWith("SoundNode"))
				NodeTree.Add(MakeShareable(Object.Get()));
		}

		TMap<USoundNode*, TSharedRef<FJsonObject>> NodeMapping;

		// Iterate through node
		for (TSharedRef<FJsonObject> NodeReference : NodeTree) {
			FString Type = NodeReference->GetStringField("Type");
			FString Name = NodeReference->GetStringField("Name");
			TSharedPtr<FJsonObject> NodeProperties = NodeReference->GetObjectField("Properties");

			// Construct sound node
			const UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Type);
			USoundNode* SoundNode = NewObject<USoundNode>(SoundCue, Class, FName(*Name), RF_Transactional);

			SoundCue->AllNodes.Add(SoundNode);
			SoundCue->SetupSoundNode(SoundNode, false);

			if (SoundNode->GetMaxChildNodes() > 0 && SoundNode->ChildNodes.Num() == 0) {
				SoundNode->CreateStartingConnectors();
			}

			SoundNode->GraphNode->CreateNewGuid();
			NodeMapping.Add(SoundNode, NodeReference);
		}

		// Set Properties, and connections
		for (TTuple<USoundNode*, TSharedRef<FJsonObject>>& Key : NodeMapping) {
			TSharedPtr<FJsonObject> NodeProperties = Key.Value->GetObjectField("Properties");
			USoundNode* SoundNode = Key.Key;

			if (USoundNodeWavePlayer* SoundWavePlayer; Cast<USoundNodeWavePlayer>(SoundNode)) {
				SoundWavePlayer = Cast<USoundNodeWavePlayer>(SoundNode);

				if (const TSharedPtr<FJsonObject>* SoundWave; NodeProperties->TryGetObjectField("SoundWaveAssetPtr", SoundWave)) {
					TObjectPtr<USoundWave> Wave; {
						LoadObject(SoundWave, Wave);
					}

					SoundWavePlayer->SetSoundWave(Wave);
				}
			}

			if (const TArray<TSharedPtr<FJsonValue>>* ChildNodesPtr; NodeProperties->TryGetArrayField("ChildNodes", ChildNodesPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> ChildValue : *ChildNodesPtr) {
					const TSharedPtr<FJsonObject>* Obj = &ChildValue->AsObject();

					SoundNode->InsertChildNode(i);
					LoadObject(Obj, SoundNode->ChildNodes[i]);

					UEdGraphNode* OutNodeGraph = SoundNode->GetGraphNode();
					check(OutNodeGraph);
					UEdGraphPin* OutputPin = nullptr;
					for (UEdGraphPin* ActivePin : SoundNode->ChildNodes[i]->GetGraphNode()->Pins)
					{
						if (ActivePin->Direction == EGPD_Output)
						{
							OutputPin = ActivePin;
							break;
						}
					}

					UEdGraphPin* InputPin = nullptr;
					int32 Inputs = 0;
					for (UEdGraphPin* ActivePin : OutNodeGraph->Pins)
					{
						if (ActivePin && ActivePin->Direction == EGPD_Input)
						{
							if (Inputs == i)
							{
								InputPin = ActivePin;
								break;
							}
							Inputs++;
						}
					}

					if (InputPin)
					{
						InputPin->MakeLinkTo(OutputPin);
					}
					i++;
				}
			}

			GetObjectSerializer()->DeserializeObjectProperties(NodeProperties, SoundNode);
		}

		GetObjectSerializer()->DeserializeObjectProperties(Properties, SoundCue);
		SoundCue->CompileSoundNodesFromGraphNodes();
		SoundCue->LinkGraphNodesFromSoundNodes();
		SoundCue->PostEditChange();
		SoundCue->MarkPackageDirty();

		HandleAssetCreation(SoundCue);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
