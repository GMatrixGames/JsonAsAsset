// Copyright JAA Contributors 2023-2024

#include "Importers/SkeletonImporter.h"

#include "Animation/BlendProfile.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Factories/TextureFactory.h"
#include "Utilities/AssetUtilities.h"
#include "Utilities/MathUtilities.h"

bool USkeletonAssetDerived::AddVirtualBone(const FName SourceBoneName, const FName TargetBoneName, const FName VirtualBoneRootName) {
	for (const FVirtualBone& SSBone : VirtualBones) {
		if (SSBone.SourceBoneName == SourceBoneName && SSBone.TargetBoneName == TargetBoneName) {
			return false;
		}
	}

	Modify();

	FVirtualBone VirtualBone = FVirtualBone(SourceBoneName, TargetBoneName);
	VirtualBone.VirtualBoneName = VirtualBoneRootName;

	VirtualBones.Add(VirtualBone);

	VirtualBoneGuid = FGuid::NewGuid();
	check(VirtualBoneGuid.IsValid());

	return true;
}

bool USkeletonImporter::ImportData() {
	try {
		bool ImportSlotGroups = true;
		bool ImportBlendProfiles = true;
		bool ImportSockets = true;
		bool ImportVirtualBones = true;
		bool ImportRetargetingModes = true;

		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		UObject* Asset = FAssetUtilities::GetSelectedAsset();
		if (!Asset) return false;

		if (USkeleton* Skeleton = Cast<USkeleton>(Asset)) {
			bool bChangeGUID = false;

			if (ImportRetargetingModes) {
				UTextureFactory* TextureFactory = NewObject<UTextureFactory>();

				for (int i = 0; i < Properties->GetArrayField("BoneTree").Num(); i++) {
					const TSharedPtr<FJsonObject> BoneNode = Properties->GetArrayField("BoneTree")[i]->AsObject();
					FString TranslationRetargetingMode = BoneNode->GetStringField("TranslationRetargetingMode");
					Skeleton->SetBoneTranslationRetargetingMode(i, static_cast<EBoneTranslationRetargetingMode::Type>(StaticEnum<EBoneTranslationRetargetingMode::Type>()->GetValueByNameString(TranslationRetargetingMode)), false);
				}
			}

			if (ImportSlotGroups) {
				for (const TSharedPtr<FJsonValue> SlotGroupValue : Properties->GetArrayField("SlotGroups")) {
					const TSharedPtr<FJsonObject> SlotGroupObject = SlotGroupValue->AsObject();

					FString GroupName = SlotGroupObject->GetStringField("GroupName");
					TArray<TSharedPtr<FJsonValue>> SlotNamesArray = SlotGroupObject->GetArrayField("SlotNames");

					for (const TSharedPtr<FJsonValue> SlotName : SlotNamesArray) {
						Skeleton->Modify();
						Skeleton->SetSlotGroupName(FName(*SlotName->AsString()), FName(*GroupName));
					}
				}
			}

			if (ImportVirtualBones) {
				for (const TSharedPtr<FJsonValue> VirtualBoneValue : Properties->GetArrayField("VirtualBones")) {
					const TSharedPtr<FJsonObject> VirtualBoneObject = VirtualBoneValue->AsObject();

					Cast<USkeletonAssetDerived>(Skeleton)->AddVirtualBone(FName(*VirtualBoneObject->GetStringField("SourceBoneName")),
					                                                      FName(*VirtualBoneObject->GetStringField("TargetBoneName")),
					                                                      FName(*VirtualBoneObject->GetStringField("VirtualBoneName")));
				}
			}

			for (const TSharedPtr<FJsonValue> SecondaryPurposeValueObject : AllJsonObjects) {
				const TSharedPtr<FJsonObject> SecondaryPurposeObject = SecondaryPurposeValueObject->AsObject();

				FString SecondaryPurposeType = SecondaryPurposeObject->GetStringField("Type");
				FString SecondaryPurposeName = SecondaryPurposeObject->GetStringField("Name");

				if (SecondaryPurposeType == "BlendProfile" && ImportBlendProfiles) {
					const TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");
					bool bIsAlreadyCreated = false;

					for (const UBlendProfile* BlendProfileC : Skeleton->BlendProfiles) {
						if (!bIsAlreadyCreated) {
							bIsAlreadyCreated = BlendProfileC->GetFName() == FName(*SecondaryPurposeName);
						}
					}

					if (!bIsAlreadyCreated) {
						Skeleton->Modify();

						UBlendProfile* BlendProfile = NewObject<UBlendProfile>(Skeleton, *SecondaryPurposeName, RF_Public | RF_Transactional);
						Skeleton->BlendProfiles.Add(BlendProfile);

						for (const TSharedPtr<FJsonValue> ProfileEntryValue : SecondaryPurposeProperties->GetArrayField("ProfileEntries")) {
							const TSharedPtr<FJsonObject> ProfileEntry = ProfileEntryValue->AsObject();

							Skeleton->Modify();

#if ENGINE_MAJOR_VERSION < 5
							BlendProfile->SetBoneBlendScale(FName(*ProfileEntry->GetObjectField("BoneReference")->GetStringField("BoneName")), ProfileEntry->GetNumberField("BlendScale"), false, true);
#endif
						}
					}
				}

				if (SecondaryPurposeType == "SkeletalMeshSocket" && ImportSockets) {
					TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");

					FString SocketName = SecondaryPurposeProperties->GetStringField("SocketName");
					FString BoneName = SecondaryPurposeProperties->GetStringField("BoneName");

					USkeletalMeshSocket* Socket = NewObject<USkeletalMeshSocket>(Skeleton);
					Socket->SocketName = FName(*SocketName);
					Socket->BoneName = FName(*BoneName);

					bool bIsAlreadyCreated = false;

					for (USkeletalMeshSocket* SocketO : Skeleton->Sockets) {
						if (!bIsAlreadyCreated) {
							bIsAlreadyCreated = SocketO->SocketName == FName(*SecondaryPurposeName);
						}
					}

					if (!bIsAlreadyCreated) {
						if (const TSharedPtr<FJsonObject>* RelativeRotationObjectRotator; SecondaryPurposeProperties->TryGetObjectField("RelativeRotation", RelativeRotationObjectRotator) == true)
							Socket->RelativeRotation = FMathUtilities::ObjectToRotator(RelativeRotationObjectRotator->Get());
						if (const TSharedPtr<FJsonObject>* RelativeLocationObjectVector; SecondaryPurposeProperties->TryGetObjectField("RelativeLocation", RelativeLocationObjectVector) == true)
							Socket->RelativeLocation = FMathUtilities::ObjectToVector(RelativeLocationObjectVector->Get());
						if (const TSharedPtr<FJsonObject>* RelativeScaleObjectVector; SecondaryPurposeProperties->TryGetObjectField("RelativeScale", RelativeScaleObjectVector) == true)
							Socket->RelativeScale = FMathUtilities::ObjectToVector(RelativeScaleObjectVector->Get());

						if (bool bForceAlwaysAnimated; SecondaryPurposeProperties->TryGetBoolField("RelativeScale", bForceAlwaysAnimated) == true)
							Socket->bForceAlwaysAnimated = bForceAlwaysAnimated;

						Skeleton->Modify();
						Skeleton->Sockets.Add(Socket);
					}
				}
			}
		}
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}
