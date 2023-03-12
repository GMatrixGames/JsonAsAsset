// Copyright Epic Games, Inc. All Rights Reserved.

#include "JsonAsAsset.h"
#include "JsonAsAssetStyle.h"
#include "JsonAsAssetCommands.h"

#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Runtime/Engine/Classes/Engine/World.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMeshActor.h"

#include "Factories/TextureFactory.h"

#include "Misc/MessageDialog.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "ToolMenus.h"

// | ------------------------------------------------------

// ----> Asset Classes
#include "Animation/BlendProfile.h"
#include "Animation/MorphTarget.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimTypes.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BoxElem.h"
#include "PhysicsEngine/ConstraintTypes.h"
#include "PhysicsEngine/SphylElem.h"
#include "PhysicsEngine/SphereElem.h"
#include "PhysicsEngine/ShapeElem.h"
#include "PhysicsEngine/TaperedCapsuleElem.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Engine/SkeletalMeshLODSettings.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SubsurfaceProfile.h"
#include "Curves/CurveLinearColor.h"
#include "Sound/ReverbEffect.h"

// ----> Factories
#include "Factories/CurveFactory.h"
#include "Factories/DataAssetFactory.h"
#include "Distributions.h"
#include "DerivedAssets/SkeletalMeshLODSettingsDerived.h"
#include "Importers/CurveFloatImporter.h"
#include "Importers/CurveLinearColorImporter.h"
#include "Importers/Importer.h"
#include "Importers/SkeletalMeshLODSettingsImporter.h"

// ------------------------------------------------------ |

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

void FJsonAsAssetModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FJsonAsAssetStyle::Initialize();
	FJsonAsAssetStyle::ReloadTextures();

	FJsonAsAssetCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FJsonAsAssetCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJsonAsAssetModule::RegisterMenus));
}

void FJsonAsAssetModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FJsonAsAssetStyle::Shutdown();

	FJsonAsAssetCommands::Unregister();
}

void FJsonAsAssetModule::PluginButtonClicked()
{
	TArray<FString> OutFileNames;

	void* ParentWindowHandle = nullptr;
	bool bIsUnrealEngine5 = false;

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();

	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid())
	{
		ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		uint32 SelectionFlag = 0;
		DesktopPlatform->OpenFileDialog(ParentWindowHandle, TEXT("Open a JSON file"), TEXT(""), FString(""), TEXT("JSON Files|*.json"), SelectionFlag,
		                                OutFileNames);
	}

	if (OutFileNames.Num() != 0 && OutFileNames[0] != "")
	{
		FString ContentBefore;
		FFileHelper::LoadFileToString(ContentBefore, *OutFileNames[0]);

		// For some reason a array
		// without it being in a object.
		FString Content = FString(TEXT("{\"data\": "));
		Content.Append(ContentBefore);
		Content.Append(FString("}"));

		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			GLog->Log("JsonAsAsset: Deserialized file, reading the contents.");

			TArray<FString> Types;
			TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField("data");
			for (TSharedPtr<FJsonValue>& Obj : DataObjects)
			{
				Types.Add(Obj->AsObject()->GetStringField("Type"));
			}

			if (!IImporter::CanImportAny(Types))
			{
				FText DialogText = FText::FromString("No exports from \"" + OutFileNames[0] + "\" can be imported!");
				FMessageDialog::Open(EAppMsgType::Ok, DialogText);
			}
			
			for (TSharedPtr<FJsonValue>& Obj : DataObjects)
			{
				TSharedPtr<FJsonObject> DataObject = Obj->AsObject();

				FString Type = DataObject->GetStringField("Type");
				FString Name = DataObject->GetStringField("Name");

				if (IImporter::CanImport(Type))
				{
					UPackage* OutermostPkg;
					UPackage* Package = CreateAssetPackage(Name, OutFileNames, OutermostPkg);
					IImporter* Importer;

					// Curves
					if (Type == "CurveFloat") Importer = new UCurveFloatImporter(Name, DataObject, Package, OutermostPkg);
					// else if (Type == "CurveVector") Importer = new UCurveVectorImporter(Name, DataObject, Package, OutermostPkg); // TODO
					else if (Type == "CurveLinearColor") Importer = new UCurveLinearColorImporter(Name, DataObject, Package, OutermostPkg);

					// Skeletal
					else if (Type == "SkeletalMeshLODSettings") Importer = new USkeletalMeshLODSettingsImporter(Name, DataObject, Package, OutermostPkg);
					else Importer = nullptr;

					if (Importer != nullptr && Importer->ImportData())
					{
						UE_LOG(LogJson, Log, TEXT("Successfully imported \"%s\" as \"%s\""), *Name, *Type)
					}
					else
					{
						FText DialogText = FText::FromString("The \"" + Type + "\" cannot be imported!");
						FMessageDialog::Open(EAppMsgType::Ok, DialogText);
					}
				}

				// USkeleton
				if (Type == "Skeleton")
				{
					GLog->Log("JsonAsAsset: Found a skeleton asset by the name of " + Name + ", parsing..");
					/*
					* Below is the UNUSED code to create a skeleton, however it needs a skeletal mesh as said by the error:
					*	"Must specify a valid skeletal mesh for the skeleton to target."
					*
					*		FString IMPath = OutFileNames[0];
					* 		FString Path;
					*
					*  		IMPath.Split("FortniteGame/Content/", nullptr, &Path);
					* 		Path.Split("/", &Path, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					*
					*		FString PathWithGame = "/Game/" + Path + "/" + Name;
					*		UPackage* Package = CreatePackage(NULL, *PathWithGame);
					*		UPackage* OutermostPkg = Package->GetOutermost();
					*
					*		// We create assets using these lines
					*		// and by using the Data Asset factory.
					*
					*		auto DataAssetFactory = NewObject<USkeletonFactory>();
					*		USkeleton* LODDataAsset = Cast<USkeleton>(DataAssetFactory->FactoryCreateNew(USkeleton::StaticClass(), OutermostPkg, *Name, RF_Standalone | RF_Public, NULL, GWarn));
					*		FAssetRegistryModule::AssetCreated(LODDataAsset);
					*		LODDataAsset->MarkPackageDirty();
					*		Package->SetDirtyFlag(true);
					*		LODDataAsset->PostEditChange();
					*		LODDataAsset->AddToRoot();
					*/

					// We cannot create a skeleton at the moment
					// so for now we just add to the skeleton, which
					// is good enough for now.
					FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
					TArray<FAssetData> SelectedAssets;
					ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

					bool ImportSlotGroups = true;
					bool ImportBlendProfiles = true;
					bool ImportSockets = true;
					bool ImportVirtualBones = true;
					bool ImportRetargetingModes = true;

					// Properties of the object
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					UObject* Asset = GetSelectedAsset();

					// If there is no asset selected
					// we do not do anything.
					//
					// Going ahead with any of our function
					// calls, can cause errors.
					if (!Asset)
						return;

					if (USkeleton* Skeleton = Cast<USkeleton>(Asset))
					{
						bool bChangeGUID = false;

						// We want to iterate through the object
						// of the animation retargeting poses
						// because it is not a array, instead a object.
						/*for (TPair<FString, TSharedPtr<FJsonValue>> Pair : DataObject->GetObjectField("AnimRetargetSources").Get()->Values)
						{
							GLog->Log("JsonAsAsset: Found a retarget source: " + Pair.Key);

							FReferencePose ReferencePose;
							ReferencePose.PoseName = FName(*Pair.Value.Get()->AsObject()->GetStringField("PoseName"));
							TArray<FTransform> ReferenceBonePose;
								
							for (TSharedPtr<FJsonValue> PoseReferenceObject : Pair.Value.Get()->AsObject()->GetArrayField("ReferencePose")) {
								TSharedPtr<FJsonObject> PoseReference = PoseReferenceObject->AsObject();
								FTransform PoseTransform = FTransform(ObjectToQuat(PoseReference->GetObjectField("Rotation").Get()), ObjectToVector(PoseReference->GetObjectField("Translation").Get()), ObjectToVector(PoseReference->GetObjectField("Scale3D").Get()));

								ReferenceBonePose.Add(PoseTransform);
							}
							ReferencePose.ReferencePose = ReferenceBonePose;
							Skeleton->AnimRetargetSources.Add(FName(*Pair.Key), ReferencePose);
						}*/

						if (ImportRetargetingModes)
						{
							auto TextureFactory = NewObject<UTextureFactory>();
							// UCurveFloat* CurveAsset = Cast<UCurveFloat>(
							// 	CurveFactory->FactoryCreateNew(UCurveFloat::StaticClass(), OutermostPkg, *Name, RF_Standalone | RF_Public, NULL,
							// 	                               GWarn));

							for (int32 BoneIndex = 0; BoneIndex < Properties->GetArrayField("BoneTree").Num(); BoneIndex++)
							{
								FJsonObject* BoneNode = Properties->GetArrayField("BoneTree")[BoneIndex]->AsObject().Get();

								Skeleton->SetBoneTranslationRetargetingMode(BoneIndex,
								                                            BoneNode->GetStringField("TranslationRetargetingMode").EndsWith(
									                                            "Animation")
									                                            ? EBoneTranslationRetargetingMode::Animation
									                                            : BoneNode->GetStringField("TranslationRetargetingMode").EndsWith(
										                                            "OrientAndScale")
									                                            ? EBoneTranslationRetargetingMode::OrientAndScale
									                                            : BoneNode->GetStringField("TranslationRetargetingMode").EndsWith(
										                                            "Skeleton")
									                                            ? EBoneTranslationRetargetingMode::Skeleton
									                                            : BoneNode->GetStringField("TranslationRetargetingMode").EndsWith(
										                                            "AnimationRelative")
									                                            ? EBoneTranslationRetargetingMode::AnimationRelative
									                                            : BoneNode->GetStringField("TranslationRetargetingMode").EndsWith(
										                                            "AnimationScaled")
									                                            ? EBoneTranslationRetargetingMode::AnimationScaled
									                                            : EBoneTranslationRetargetingMode::Animation, false);
							}
						}

						// Import the slot groups + names
						// 
						// Does not override the current slot groups, and doesn't create duplicates.
						// [Optional]
						if (ImportSlotGroups)
							for (TSharedPtr<FJsonValue> SlotGroupValue : Properties->GetArrayField("SlotGroups"))
							{
								// Get the object
								TSharedPtr<FJsonObject> SlotGroupObject = SlotGroupValue->AsObject();

								FString GroupName = SlotGroupObject->GetStringField("GroupName");
								TArray<TSharedPtr<FJsonValue>> SlotNamesArray = SlotGroupObject->GetArrayField("SlotNames");

								// Loop through each slot to add
								for (TSharedPtr<FJsonValue> SlotName : SlotNamesArray)
								{
									Skeleton->Modify();

									// Set the group name and slot name (the function also creates it if it doesn't exist!)
									Skeleton->SetSlotGroupName(FName(*SlotName->AsString()), FName(*GroupName));
								}
							}

						// Import the virtual bones
						// [Optional]
						if (ImportVirtualBones)
							for (TSharedPtr<FJsonValue> VirtualBoneValue : Properties->GetArrayField("VirtualBones"))
							{
								TSharedPtr<FJsonObject> VirtualBoneObject = VirtualBoneValue->AsObject();

								// Cast<SkeletonAssetDerived>(Skeleton)->AddVirtualBone(FName(*VirtualBoneObject->GetStringField("SourceBoneName")),
								//                                                      FName(*VirtualBoneObject->GetStringField("TargetBoneName")),
								//                                                      FName(*VirtualBoneObject->GetStringField("VirtualBoneName")));
							}

						for (TSharedPtr<FJsonValue> SecondaryPurposeValueObject : DataObjects)
						{
							TSharedPtr<FJsonObject> SecondaryPurposeObject = SecondaryPurposeValueObject->AsObject();

							FString SecondaryPurposeType = SecondaryPurposeObject->GetStringField("Type");
							FString SecondaryPurposeName = SecondaryPurposeObject->GetStringField("Name");

							// Import all blend profiles
							if (SecondaryPurposeType == "BlendProfile" && ImportBlendProfiles)
							{
								// Properties of the object
								TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");
								bool bIsAlreadyCreated = false;

								// Check if the blend profile was already created.
								for (UBlendProfile* BlendProfileC : Skeleton->BlendProfiles)
									if (!bIsAlreadyCreated)
										bIsAlreadyCreated = BlendProfileC->GetFName() == FName(*SecondaryPurposeName);

								if (!bIsAlreadyCreated)
								{
									Skeleton->Modify();

									UBlendProfile* BlendProfile = NewObject<UBlendProfile>(
										Skeleton, *SecondaryPurposeName, RF_Public | RF_Transactional);
									Skeleton->BlendProfiles.Add(BlendProfile);

									for (TSharedPtr<FJsonValue> ProfileEntryValue : SecondaryPurposeProperties->GetArrayField("ProfileEntries"))
									{
										TSharedPtr<FJsonObject> ProfileEntry = ProfileEntryValue->AsObject();

										Skeleton->Modify();

										// Set the bone scale using the properties
										// provided in the JSON file.
										if (!bIsUnrealEngine5)
											BlendProfile->SetBoneBlendScale(
												FName(*ProfileEntry->GetObjectField("BoneReference")->GetStringField("BoneName")),
												ProfileEntry->GetNumberField("BlendScale"), false, true);
									}
								}
							}

							// Import all sockets
							if (SecondaryPurposeType == "SkeletalMeshSocket" && ImportSockets)
							{
								// Properties of the object
								TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");

								FString SocketName = SecondaryPurposeProperties->GetStringField("SocketName");
								FString BoneName = SecondaryPurposeProperties->GetStringField("BoneName");

								USkeletalMeshSocket* Socket = NewObject<USkeletalMeshSocket>(Skeleton);
								Socket->SocketName = FName(*SocketName);
								Socket->BoneName = FName(*BoneName);

								bool bIsAlreadyCreated = false;

								// Check if the blend profile was already created.
								for (USkeletalMeshSocket* SocketO : Skeleton->Sockets)
									if (!bIsAlreadyCreated)
										bIsAlreadyCreated = SocketO->SocketName == FName(*SecondaryPurposeName);

								if (!bIsAlreadyCreated)
								{
									const TSharedPtr<FJsonObject>* RelativeLocationObjectVector;
									const TSharedPtr<FJsonObject>* RelativeScaleObjectVector;
									const TSharedPtr<FJsonObject>* RelativeRotationObjectRotator;
									bool bForceAlwaysAnimated;

									// Since the location, scale and rotation are completely
									// optional they are not set in the object if they aren't
									// modified at all.

									// Example:
									//
									// You make a socket but do not set anything, then when
									// you look at the properties in FModel, you'll see that
									// there is no variable even defined.
									//
									// But try with a new socket and modify everything (location,
									// rotation, scale) and seeing the programs you'll find those
									// variables being set, unlike the socket without any modifications.

									if (SecondaryPurposeProperties->TryGetObjectField("RelativeRotation", RelativeRotationObjectRotator) == true)
										Socket->RelativeRotation = ObjectToRotator(RelativeRotationObjectRotator->Get());
									if (SecondaryPurposeProperties->TryGetObjectField("RelativeLocation", RelativeLocationObjectVector) == true)
										Socket->RelativeLocation = ObjectToVector(RelativeLocationObjectVector->Get());
									if (SecondaryPurposeProperties->TryGetObjectField("RelativeScale", RelativeScaleObjectVector) == true)
										Socket->RelativeScale = ObjectToVector(RelativeScaleObjectVector->Get());

									// Haven't found any skeleton with this
									// but just in-case
									if (SecondaryPurposeProperties->TryGetBoolField("RelativeScale", bForceAlwaysAnimated) == true)
										Socket->bForceAlwaysAnimated = bForceAlwaysAnimated;

									Skeleton->Modify();

									// Finally we add the socket to the 
									// socket array.
									Skeleton->Sockets.Add(Socket);
								}
							}
						}
					}
				}
				// USubsurfaceProfile
				else if (Type == "SubsurfaceProfile")
				{
					// The path that the file
					// was imported from (RAW)
					FString IMPath = OutFileNames[0];
					FString Path;

					IMPath.Split("FortniteGame/Content/", nullptr, &Path);
					Path.Split("/", &Path, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

					FString PathWithGame = "/Game/" + Path + "/" + Name;
					UPackage* Package = CreatePackage(NULL, *PathWithGame);
					UPackage* OutermostPkg = Package->GetOutermost();
					Package->FullyLoad();

					USubsurfaceProfile* SubsurfaceProfile = NewObject<USubsurfaceProfile>(Package, USubsurfaceProfile::StaticClass(), *Name,
					                                                                      EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

					// Settings of the subsurface profile
					TSharedPtr<FJsonObject> Settings = DataObject->GetObjectField("Properties")->GetObjectField("Settings");
					FSubsurfaceProfileStruct Profile;

					bool bEnableBurley;
					const TSharedPtr<FJsonObject>* BoundaryColorBleed;
					int64 ExtinctionScale;
					const TSharedPtr<FJsonObject>* FalloffColor;
					int64 IOR;
					int64 LobeMix;
					const TSharedPtr<FJsonObject>* MeanFreePathColor;
					int64 MeanFreePathDistance;
					int64 NormalScale;
					int64 Roughness0;
					int64 Roughness1;
					int64 ScatteringDistribution;
					int64 ScatterRadius;
					const TSharedPtr<FJsonObject>* SubsurfaceColor;
					const TSharedPtr<FJsonObject>* SurfaceAlbedo;
					const TSharedPtr<FJsonObject>* TransmissionTintColor;
					int64 WorldUnitScale;

					if (Settings->TryGetBoolField("bEnableBurley", bEnableBurley))
						Profile.bEnableBurley = bEnableBurley;

					// Linear Color
					if (Settings->TryGetObjectField("BoundaryColorBleed", BoundaryColorBleed))
						Profile.BoundaryColorBleed = ObjectToLinearColor(BoundaryColorBleed->Get());
					if (Settings->TryGetObjectField("MeanFreePathColor", MeanFreePathColor))
						Profile.MeanFreePathColor = ObjectToLinearColor(MeanFreePathColor->Get());
					if (Settings->TryGetObjectField("FalloffColor", FalloffColor))
						Profile.FalloffColor = ObjectToLinearColor(FalloffColor->Get());
					if (Settings->TryGetObjectField("TransmissionTintColor", TransmissionTintColor))
						Profile.TransmissionTintColor = ObjectToLinearColor(TransmissionTintColor->Get());
					if (Settings->TryGetObjectField("SubsurfaceColor", SubsurfaceColor))
						Profile.SubsurfaceColor = ObjectToLinearColor(SubsurfaceColor->Get());
					if (Settings->TryGetObjectField("SurfaceAlbedo", SurfaceAlbedo))
						Profile.SurfaceAlbedo = ObjectToLinearColor(SurfaceAlbedo->Get());

					if (Settings->TryGetNumberField("ExtinctionScale", ExtinctionScale))
						Profile.ExtinctionScale = Settings->GetNumberField("ExtinctionScale");
					if (Settings->TryGetNumberField("IOR", IOR))
						Profile.IOR = Settings->GetNumberField("IOR");
					if (Settings->TryGetNumberField("LobeMix", LobeMix))
						Profile.LobeMix = Settings->GetNumberField("LobeMix");
					if (Settings->TryGetNumberField("MeanFreePathDistance", MeanFreePathDistance))
						Profile.MeanFreePathDistance = Settings->GetNumberField("MeanFreePathDistance");
					if (Settings->TryGetNumberField("NormalScale", NormalScale))
						Profile.NormalScale = Settings->GetNumberField("NormalScale");
					if (Settings->TryGetNumberField("Roughness0", Roughness0))
						Profile.Roughness0 = Settings->GetNumberField("Roughness0");
					if (Settings->TryGetNumberField("Roughness1", Roughness1))
						Profile.Roughness1 = Settings->GetNumberField("Roughness1");
					if (Settings->TryGetNumberField("ScatteringDistribution", ScatteringDistribution))
						Profile.ScatteringDistribution = Settings->GetNumberField("ScatteringDistribution");
					if (Settings->TryGetNumberField("ScatterRadius", ScatterRadius))
						Profile.ScatterRadius = Settings->GetNumberField("ScatterRadius");
					if (Settings->TryGetNumberField("WorldUnitScale", WorldUnitScale))
						Profile.WorldUnitScale = Settings->GetNumberField("WorldUnitScale");

					SubsurfaceProfile->Settings = Profile;

					FAssetRegistryModule::AssetCreated(SubsurfaceProfile);
					SubsurfaceProfile->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					SubsurfaceProfile->PostEditChange();
					SubsurfaceProfile->AddToRoot();
				}
				// USkeletalMesh
				else if (Type == "PhysicsAsset")
				{
					GLog->Log("JsonAsAsset: Found a physics asset by the name of " + Name + ", parsing..");

					// We cannot create a skeletal mesh at the moment
					// so for now we just add to the skeletal mesh, which
					// is good enough for now.
					FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
					TArray<FAssetData> SelectedAssets;
					ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

					// Properties of the object
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					for (FAssetData& AssetData : SelectedAssets)
					{
						UObject* Asset = AssetData.GetAsset();

						if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
						{
							bool bChangeGUID = false;

							// The path that the file
							// was imported from (RAW)
							FString IMPath = OutFileNames[0];
							FString Path;

							IMPath.Split("FortniteGame/Content/", nullptr, &Path);
							Path.Split("/", &Path, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

							FString PathWithGame = "/Game/" + Path + "/" + Name;
							UPackage* Package = CreatePackage(NULL, *PathWithGame);
							UPackage* OutermostPkg = Package->GetOutermost();
							Package->FullyLoad();

							UPhysicsAsset* PhysAsset = NewObject<UPhysicsAsset>(Package, UPhysicsAsset::StaticClass(), *Name,
							                                                    EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

							PhysAsset->SetPreviewMesh(SkeletalMesh);

							for (TSharedPtr<FJsonValue> SecondaryPurposeValueObject : DataObjects)
							{
								TSharedPtr<FJsonObject> SecondaryPurposeObject = SecondaryPurposeValueObject->AsObject();

								FString SecondaryPurposeType = SecondaryPurposeObject->GetStringField("Type");
								FString SecondaryPurposeName = SecondaryPurposeObject->GetStringField("Name");

								int32 anIndex = 0;

								// Import all sockets
								if (SecondaryPurposeType == "SkeletalBodySetup")
								{
									// Properties of the object
									TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");
									USkeletalBodySetup* NewBodySetup = NewObject<USkeletalBodySetup>(PhysAsset, NAME_None, RF_Transactional);

									NewBodySetup->BodySetupGuid = FGuid(*SecondaryPurposeObject->GetStringField("BodySetupGuid"));
									NewBodySetup->BoneName = FName(*SecondaryPurposeProperties->GetStringField("BoneName"));

									FString PhysicsType;
									FString CollisionTraceFlag;
									FString CollisionReponse;

									if (SecondaryPurposeProperties->TryGetStringField("PhysicsType", PhysicsType))
									{
										NewBodySetup->PhysicsType =
											PhysicsType.EndsWith("PhysType_Default")
												? EPhysicsType::PhysType_Default
												: PhysicsType.EndsWith("PhysType_Kinematic")
												? EPhysicsType::PhysType_Kinematic
												: PhysicsType.EndsWith("PhysType_Simulated")
												? EPhysicsType::PhysType_Simulated
												: EPhysicsType::PhysType_Default;
									}
									if (SecondaryPurposeProperties->TryGetStringField("CollisionReponse", CollisionReponse))
									{
										NewBodySetup->CollisionReponse =
											CollisionReponse.EndsWith("BodyCollision_Enabled")
												? EBodyCollisionResponse::BodyCollision_Enabled
												: CollisionReponse.EndsWith("BodyCollision_Disabled")
												? EBodyCollisionResponse::BodyCollision_Disabled
												: EBodyCollisionResponse::BodyCollision_Enabled;
									}
									if (SecondaryPurposeProperties->TryGetStringField("CollisionTraceFlag", CollisionTraceFlag))
									{
										NewBodySetup->CollisionTraceFlag =
											PhysicsType.EndsWith("CTF_UseDefault")
												? ECollisionTraceFlag::CTF_UseDefault
												: PhysicsType.EndsWith("CTF_UseSimpleAndComplex")
												? ECollisionTraceFlag::CTF_UseSimpleAndComplex
												: PhysicsType.EndsWith("CTF_UseSimpleAndComplex")
												? ECollisionTraceFlag::CTF_UseSimpleAndComplex
												: PhysicsType.EndsWith("CTF_UseSimpleAsComplex")
												? ECollisionTraceFlag::CTF_UseSimpleAsComplex
												: PhysicsType.EndsWith("CTF_UseComplexAsSimple")
												? ECollisionTraceFlag::CTF_UseComplexAsSimple
												: ECollisionTraceFlag::CTF_UseDefault;
									}

									const TSharedPtr<FJsonObject>* AggGeom;
									if (SecondaryPurposeProperties->TryGetObjectField("AggGeom", AggGeom))
									{
										const TArray<TSharedPtr<FJsonValue>>* SphylElems;
										FKAggregateGeom AggOem;

										if (AggGeom->Get()->TryGetArrayField("SphylElems", SphylElems))
										{
											for (TSharedPtr<FJsonValue> SphylElemOb : AggGeom->Get()->GetArrayField("SphylElems"))
											{
												TSharedPtr<FJsonObject> SphylElemObject = SphylElemOb->AsObject();
												FKSphylElem SphylElem;

												SphylElem.Center = ObjectToVector(SphylElemObject->GetObjectField("Center").Get());
												SphylElem.Rotation = ObjectToRotator(SphylElemObject->GetObjectField("Rotation").Get());

												SphylElem.Radius = SphylElemObject->GetNumberField("Radius");
												SphylElem.Length = SphylElemObject->GetNumberField("Length");
												SphylElem.RestOffset = SphylElemObject->GetNumberField("RestOffset");
												SphylElem.SetName(FName(*SphylElemObject->GetStringField("Name")));
												SphylElem.SetContributeToMass(SphylElemObject->GetBoolField("bContributeToMass"));
												AggOem.SphylElems.Add(SphylElem);
											}
										}

										if (AggGeom->Get()->TryGetArrayField("SphereElems", SphylElems))
										{
											for (TSharedPtr<FJsonValue> SphylElemOb : AggGeom->Get()->GetArrayField("SphereElems"))
											{
												TSharedPtr<FJsonObject> SphylElemObject = SphylElemOb->AsObject();
												FKSphereElem SphylElem;

												SphylElem.Center = ObjectToVector(SphylElemObject->GetObjectField("Center").Get());

												SphylElem.Radius = SphylElemObject->GetNumberField("Radius");
												SphylElem.RestOffset = SphylElemObject->GetNumberField("RestOffset");
												SphylElem.SetName(FName(*SphylElemObject->GetStringField("Name")));
												SphylElem.SetContributeToMass(SphylElemObject->GetBoolField("bContributeToMass"));
												AggOem.SphereElems.Add(SphylElem);
											}
										}

										if (AggGeom->Get()->TryGetArrayField("BoxElems", SphylElems))
										{
											for (TSharedPtr<FJsonValue> SphylElemOb : AggGeom->Get()->GetArrayField("BoxElems"))
											{
												TSharedPtr<FJsonObject> SphylElemObject = SphylElemOb->AsObject();
												FKBoxElem SphylElem;

												SphylElem.Center = ObjectToVector(SphylElemObject->GetObjectField("Center").Get());
												SphylElem.Rotation = ObjectToRotator(SphylElemObject->GetObjectField("Rotation").Get());

												SphylElem.X = SphylElemObject->GetNumberField("X");
												SphylElem.Y = SphylElemObject->GetNumberField("Y");
												SphylElem.Z = SphylElemObject->GetNumberField("Z");
												SphylElem.RestOffset = SphylElemObject->GetNumberField("RestOffset");
												SphylElem.SetName(FName(*SphylElemObject->GetStringField("Name")));
												SphylElem.SetContributeToMass(SphylElemObject->GetBoolField("bContributeToMass"));
												AggOem.BoxElems.Add(SphylElem);
											}
										}

										if (AggGeom->Get()->TryGetArrayField("TaperedCapsuleElems", SphylElems))
										{
											for (TSharedPtr<FJsonValue> TaperedCapsuleElemOb : AggGeom->Get()->GetArrayField("TaperedCapsuleElems"))
											{
												TSharedPtr<FJsonObject> TaperedCapsuleElemObject = TaperedCapsuleElemOb->AsObject();
												FKTaperedCapsuleElem TaperedCapsuleElem;

												TaperedCapsuleElem.Center = ObjectToVector(TaperedCapsuleElemObject->GetObjectField("Center").Get());
												TaperedCapsuleElem.Rotation = ObjectToRotator(
													TaperedCapsuleElemObject->GetObjectField("Rotation").Get());

												TaperedCapsuleElem.Radius0 = TaperedCapsuleElemObject->GetNumberField("Radius0");
												TaperedCapsuleElem.Radius1 = TaperedCapsuleElemObject->GetNumberField("Radius1");
												TaperedCapsuleElem.RestOffset = TaperedCapsuleElemObject->GetNumberField("RestOffset");

												AggOem.TaperedCapsuleElems.Add(TaperedCapsuleElem);
											}
										}

										NewBodySetup->AggGeom = AggOem;
									}

									const TSharedPtr<FJsonObject>* DefaultInstance;
									if (SecondaryPurposeProperties->TryGetObjectField("DefaultInstance", DefaultInstance))
									{
										FBodyInstance BodyInstance;

										int64 MassInKgOverride;
										int64 AngularDamping;
										int64 LinearDamping;
										int64 PositionSolverIterationCount;
										int64 VelocitySolverIterationCount;
										const TSharedPtr<FJsonObject>* InertiaTensorScale;
										const TSharedPtr<FJsonObject>* COMNudge;

										if (DefaultInstance->Get()->TryGetNumberField("MassInKgOverride", MassInKgOverride))
										{
											BodyInstance.SetMassOverride(DefaultInstance->Get()->GetNumberField("MassInKgOverride"), true);
										}
										if (DefaultInstance->Get()->TryGetNumberField("AngularDamping", AngularDamping))
										{
											BodyInstance.AngularDamping = DefaultInstance->Get()->GetNumberField("AngularDamping");
										}
										if (DefaultInstance->Get()->TryGetNumberField("LinearDamping", LinearDamping))
										{
											BodyInstance.LinearDamping = DefaultInstance->Get()->GetNumberField("LinearDamping");
										}
										if (DefaultInstance->Get()->TryGetNumberField("PositionSolverIterationCount", PositionSolverIterationCount))
										{
											BodyInstance.PositionSolverIterationCount = DefaultInstance->Get()->GetIntegerField(
												"PositionSolverIterationCount");
										}
										if (DefaultInstance->Get()->TryGetNumberField("VelocitySolverIterationCount", VelocitySolverIterationCount))
										{
											int Velo = DefaultInstance->Get()->GetIntegerField("VelocitySolverIterationCount");
											BodyInstance.VelocitySolverIterationCount = *((uint8*)&Velo);
										}
										if (DefaultInstance->Get()->TryGetObjectField("InertiaTensorScale", InertiaTensorScale))
										{
											BodyInstance.InertiaTensorScale = ObjectToVector(InertiaTensorScale->Get());
										}
										if (DefaultInstance->Get()->TryGetObjectField("COMNudge", COMNudge))
										{
											BodyInstance.COMNudge = ObjectToVector(COMNudge->Get());
										}
									}

									int32 BodySetupIndex = PhysAsset->SkeletalBodySetups.Add(NewBodySetup);

									PhysAsset->UpdateBodySetupIndexMap();
									PhysAsset->UpdateBoundsBodiesArray();
								}
								if (SecondaryPurposeType == "PhysicsConstraintTemplate")
								{
									// Properties of the object
									TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");
									UPhysicsConstraintTemplate* NewConstraintSetup = NewObject<UPhysicsConstraintTemplate>(
										PhysAsset, NAME_None, RF_Transactional);

									NewConstraintSetup->DefaultInstance.JointName = FName(
										*SecondaryPurposeProperties->GetObjectField("DefaultInstance")->GetStringField("JointName"));
									NewConstraintSetup->DefaultInstance.ConstraintBone1 = FName(
										*SecondaryPurposeProperties->GetObjectField("DefaultInstance")->GetStringField("ConstraintBone1"));
									NewConstraintSetup->DefaultInstance.ConstraintBone2 = FName(
										*SecondaryPurposeProperties->GetObjectField("DefaultInstance")->GetStringField("ConstraintBone2"));

									/*const TSharedPtr<FJsonObject>* Pos1;
									const TSharedPtr<FJsonObject>* Pos2;
									const TSharedPtr<FJsonObject>* PriAxis1;
									const TSharedPtr<FJsonObject>* PriAxis2;
									const TSharedPtr<FJsonObject>* SecAxis1;
									const TSharedPtr<FJsonObject>* SecAxis2;*/

									/*if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField("Pos2", Pos2)) {
										NewConstraintSetup->DefaultInstance.Pos2 = ObjectToVector(Pos2->Get());
									}

									if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField("Pos1", Pos1)) {
										NewConstraintSetup->DefaultInstance.Pos1 = ObjectToVector(Pos1->Get());
									}

									if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField("PriAxis1", PriAxis1)) {
										NewConstraintSetup->DefaultInstance.PriAxis1 = ObjectToVector(PriAxis1->Get());
									}

									if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField("SecAxis1", SecAxis1)) {
										NewConstraintSetup->DefaultInstance.SecAxis1 = ObjectToVector(SecAxis1->Get());
									}

									if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField("PriAxis2", PriAxis2)) {
										NewConstraintSetup->DefaultInstance.PriAxis2 = ObjectToVector(PriAxis2->Get());
									}

									if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField("SecAxis2", SecAxis2)) {
										NewConstraintSetup->DefaultInstance.SecAxis2 = ObjectToVector(SecAxis2->Get());
									}*/


									const TSharedPtr<FJsonObject>* ProfileInstance11;

									if (SecondaryPurposeProperties->GetObjectField("DefaultInstance")->TryGetObjectField(
										"ProfileInstance", ProfileInstance11))
									{
										const FJsonObject* ProfileInstance = ProfileInstance11->Get();

										const TSharedPtr<FJsonObject>* LinearLimit;

										if (ProfileInstance->TryGetObjectField("LinearLimit", LinearLimit))
										{
											bool bSoftConstraint;
											int64 ContactDistance;
											int64 Damping;
											int64 Restitution;
											int64 Stiffness;
											int64 Limit;

											if (LinearLimit->Get()->TryGetBoolField("bSoftConstraint", bSoftConstraint))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.bSoftConstraint = bSoftConstraint;
											}

											if (LinearLimit->Get()->TryGetNumberField("ContactDistance", ContactDistance))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.ContactDistance = LinearLimit->Get()->
													GetNumberField("ContactDistance");
											}

											if (LinearLimit->Get()->TryGetNumberField("Damping", Damping))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Damping = LinearLimit->Get()->
													GetNumberField("Damping");
											}

											if (LinearLimit->Get()->TryGetNumberField("Restitution", Restitution))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Restitution = LinearLimit->Get()->
													GetNumberField("Restitution");
											}

											if (LinearLimit->Get()->TryGetNumberField("Stiffness", Stiffness))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Stiffness = LinearLimit->Get()->
													GetNumberField("Stiffness");
											}

											if (LinearLimit->Get()->TryGetNumberField("Limit", Limit))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Limit = LinearLimit->Get()->
													GetNumberField("Limit");
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.XMotion =
													LinearLimit->Get()->GetStringField("XMotion").EndsWith("LCM_Free")
														? ELinearConstraintMotion::LCM_Free
														: LinearLimit->Get()->GetStringField("XMotion").EndsWith("LCM_Limited")
														? ELinearConstraintMotion::LCM_Limited
														: LinearLimit->Get()->GetStringField("XMotion").EndsWith("LCM_Locked")
														? ELinearConstraintMotion::LCM_Locked
														: ELinearConstraintMotion::LCM_Limited;

												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.YMotion =
													LinearLimit->Get()->GetStringField("YMotion").EndsWith("LCM_Free")
														? ELinearConstraintMotion::LCM_Free
														: LinearLimit->Get()->GetStringField("YMotion").EndsWith("LCM_Limited")
														? ELinearConstraintMotion::LCM_Limited
														: LinearLimit->Get()->GetStringField("YMotion").EndsWith("LCM_Locked")
														? ELinearConstraintMotion::LCM_Locked
														: ELinearConstraintMotion::LCM_Limited;

												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.ZMotion =
													LinearLimit->Get()->GetStringField("ZMotion").EndsWith("LCM_Free")
														? ELinearConstraintMotion::LCM_Free
														: LinearLimit->Get()->GetStringField("ZMotion").EndsWith("LCM_Limited")
														? ELinearConstraintMotion::LCM_Limited
														: LinearLimit->Get()->GetStringField("ZMotion").EndsWith("LCM_Locked")
														? ELinearConstraintMotion::LCM_Locked
														: ELinearConstraintMotion::LCM_Limited;
											}
										}

										const TSharedPtr<FJsonObject>* ConeLimit;
										if (ProfileInstance->TryGetObjectField("ConeLimit", ConeLimit))
										{
											int64 Swing1LimitDegrees;
											int64 Swing2LimitDegrees;
											FString Swing1Motion;
											FString Swing2Motion;

											if (ConeLimit->Get()->TryGetNumberField("Swing1LimitDegrees", Swing1LimitDegrees))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing1LimitDegrees = ConeLimit->Get()->
													GetNumberField("Swing1LimitDegrees");
											}
											if (ConeLimit->Get()->TryGetNumberField("Swing2LimitDegrees", Swing2LimitDegrees))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing2LimitDegrees = ConeLimit->Get()->
													GetNumberField("Swing2LimitDegrees");
											}

											if (ConeLimit->Get()->TryGetStringField("Swing1Motion", Swing1Motion))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing1Motion =
													ConeLimit->Get()->GetStringField("Swing1Motion").EndsWith("ACM_Free")
														? EAngularConstraintMotion::ACM_Free
														: ConeLimit->Get()->GetStringField("Swing1Motion").EndsWith("ACM_Limited")
														? EAngularConstraintMotion::ACM_Limited
														: ConeLimit->Get()->GetStringField("Swing1Motion").EndsWith("ACM_Locked")
														? EAngularConstraintMotion::ACM_Locked
														: EAngularConstraintMotion::ACM_Limited;
											}
											if (ConeLimit->Get()->TryGetStringField("Swing2Motion", Swing2Motion))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing2Motion =
													ConeLimit->Get()->GetStringField("Swing2Motion").EndsWith("ACM_Free")
														? EAngularConstraintMotion::ACM_Free
														: ConeLimit->Get()->GetStringField("Swing2Motion").EndsWith("ACM_Limited")
														? EAngularConstraintMotion::ACM_Limited
														: ConeLimit->Get()->GetStringField("Swing2Motion").EndsWith("ACM_Locked")
														? EAngularConstraintMotion::ACM_Locked
														: EAngularConstraintMotion::ACM_Limited;
											}
										}

										const TSharedPtr<FJsonObject>* TwistLimit;
										if (ProfileInstance->TryGetObjectField("TwistLimit", TwistLimit))
										{
											int64 TwistLimitDegrees;
											FString TwistMotion;

											if (TwistLimit->Get()->TryGetNumberField("TwistLimitDegrees", TwistLimitDegrees))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.TwistLimit.TwistLimitDegrees = TwistLimit->Get()->
													GetNumberField("TwistLimitDegrees");
											}

											if (TwistLimit->Get()->TryGetStringField("TwistMotion", TwistMotion))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.TwistLimit.TwistMotion =
													TwistLimit->Get()->GetStringField("TwistMotion").EndsWith("ACM_Free")
														? EAngularConstraintMotion::ACM_Free
														: TwistLimit->Get()->GetStringField("TwistMotion").EndsWith("ACM_Limited")
														? EAngularConstraintMotion::ACM_Limited
														: TwistLimit->Get()->GetStringField("TwistMotion").EndsWith("ACM_Locked")
														? EAngularConstraintMotion::ACM_Locked
														: EAngularConstraintMotion::ACM_Limited;
											}
										}

										const TSharedPtr<FJsonObject>* AngularDrive;
										if (ProfileInstance->TryGetObjectField("AngularDrive", AngularDrive))
										{
											const TSharedPtr<FJsonObject>* TwistDrive;

											const TSharedPtr<FJsonObject>* SlerpDrive;

											if (AngularDrive->Get()->TryGetObjectField("SlerpDrive", SlerpDrive))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.SlerpDrive.bEnablePositionDrive =
													SlerpDrive->Get()->GetBoolField("bEnablePositionDrive");
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.SlerpDrive.Stiffness = SlerpDrive->
													Get()->GetNumberField("Stiffness");
											}

											if (AngularDrive->Get()->TryGetObjectField("TwistDrive", TwistDrive))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.TwistDrive.Stiffness = TwistDrive->
													Get()->GetNumberField("Stiffness");
											}

											const TSharedPtr<FJsonObject>* SwingDrive;

											if (AngularDrive->Get()->TryGetObjectField("SwingDrive", SwingDrive))
											{
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.SwingDrive.Stiffness = SwingDrive->
													Get()->GetNumberField("Stiffness");
											}
										}
									}

									PhysAsset->ConstraintSetup.Add(NewConstraintSetup);
								}
							}

							FAssetRegistryModule::AssetCreated(PhysAsset);
							PhysAsset->MarkPackageDirty();
							Package->SetDirtyFlag(true);
							PhysAsset->PostEditChange();
							PhysAsset->AddToRoot();
						}
						// Skeletal Mesh Not Selected
						else
						{
							FText DialogText = FText::FromString(TEXT("Please select a skeletal mesh to modify using the plugin."));
							FMessageDialog::Open(EAppMsgType::Ok, DialogText);
						}
					}

					// Nothing Selected At All
					if (SelectedAssets.Num() == 0)
					{
						FText DialogText = FText::FromString(TEXT(
							"You have no selected asset, please select a Skeletal Mesh because at the moment we can only add to a skeletal mesh not make one."));
						FMessageDialog::Open(EAppMsgType::Ok, DialogText);
					}
				}
				else if (Type == "AnimSequence" || Type == "AnimMontage")
				{
					// Properties of the object
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");
					UObject* Asset = GetSelectedAsset();

					// If there is no asset selected
					// we do not do anything.
					//
					// Going ahead with any of our function
					// calls, can cause errors.
					if (!Asset)
						return;

					// Cast to a animation sequence base if found
					if (UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(Asset))
					{
						EvaluateAnimSequence(DataObject.Get(), AnimSequenceBase);
					}
				}
				else if (Type == "DataTable")
				{
					/* Properties of the object
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");
					
					UPackage* Package = CreateAssetPackage(Name, OutFileNames);
					UDataTable* DataTable = NewObject<UDataTable>(Package, UDataTable::StaticClass(), *Name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

					FString ClassName;

					Properties->GetObjectField("RowStruct").Get()->GetStringField("ObjectName").Split(" ", nullptr, &ClassName);
					FString ClassModule = Properties->GetObjectField("RowStruct").Get()->GetStringField("ObjectPath");
					UScriptStruct* ScriptStruct = FindObject<UScriptStruct>(ANY_PACKAGE, *(ClassModule + "." + ClassName));

					DataTable->RowStruct = ScriptStruct;

					for (auto pair = DataObject->GetObjectField("Rows")->Values.CreateConstIterator(); pair; ++pair) {
						const FString PairName = (*pair).Key;
						TSharedPtr<FJsonValue> PairValue = (*pair).Value;

						FTableRowBase* CurRow = reinterpret_cast<FTableRowBase*>(ScriptStruct);
						DataTable->AddRow(FName(*PairName), *CurRow);
					}

					FAssetRegistryModule::AssetCreated(DataTable);
					DataTable->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					DataTable->PostEditChange();
					DataTable->AddToRoot();
					*/

					UDataTable* Asset = Cast<UDataTable>(GetSelectedAsset());

					// Add new rows or overwrite previous rows
					for (TMap<FName, uint8*>::TConstIterator RowMapIter(Asset->GetRowMap().CreateConstIterator()); RowMapIter; ++RowMapIter)
					{
						if (ensure(RowMapIter.Value() != nullptr))
						{
							uint8* RowDataPtr = RowMapIter.Value();

							UScriptStruct* CurrentStruct = (UScriptStruct*)*RowDataPtr;

							FText DialogText = FText::FromString(CurrentStruct->GetName());
							FMessageDialog::Open(EAppMsgType::Ok, DialogText);

							//UE_LOG(LogTemp, Warning, TEXT("Row Name: %s ; ScriptStructName:- %s"), RowMapIter.Key(), CurrentStruct->GetName());
						}
					}
				}
				else if (Type == "ReverbEffect")
				{
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					UPackage* Package = CreateAssetPackage(Name, OutFileNames);
					UReverbEffect* ReverbEffect = NewObject<UReverbEffect>(Package, UReverbEffect::StaticClass(), *Name,
					                                                       EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

					int64 AirAbsorptionGainHF;
					int64 DecayHFRatio;
					int64 DecayTime;
					int64 Density;
					int64 Diffusion;
					int64 Gain;
					int64 GainHF;
					int64 LateDelay;
					int64 LateGain;
					int64 ReflectionsDelay;
					int64 ReflectionsGain;
					int64 RoomRolloffFactor;
					bool bChanged;
					bool bBypassLateReflections;
					bool bBypassEarlyReflections;

					if (Properties->TryGetNumberField("AirAbsorptionGainHF", AirAbsorptionGainHF))
					{
						ReverbEffect->AirAbsorptionGainHF = Properties->GetNumberField("AirAbsorptionGainHF");
					}
					if (Properties->TryGetNumberField("DecayHFRatio", DecayHFRatio))
					{
						ReverbEffect->DecayHFRatio = Properties->GetNumberField("DecayHFRatio");
					}
					if (Properties->TryGetNumberField("DecayTime", DecayTime))
					{
						ReverbEffect->DecayTime = Properties->GetNumberField("DecayTime");
					}
					if (Properties->TryGetNumberField("Density", Density))
					{
						ReverbEffect->Density = Properties->GetNumberField("Density");
					}
					if (Properties->TryGetNumberField("Diffusion", Diffusion))
					{
						ReverbEffect->Diffusion = Properties->GetNumberField("Diffusion");
					}
					if (Properties->TryGetNumberField("Gain", Gain))
					{
						ReverbEffect->Gain = Properties->GetNumberField("Gain");
					}
					if (Properties->TryGetNumberField("GainHF", GainHF))
					{
						ReverbEffect->GainHF = Properties->GetNumberField("GainHF");
					}
					if (Properties->TryGetNumberField("LateDelay", LateDelay))
					{
						ReverbEffect->LateDelay = Properties->GetNumberField("LateDelay");
					}
					if (Properties->TryGetNumberField("LateGain", LateGain))
					{
						ReverbEffect->LateGain = Properties->GetNumberField("LateGain");
					}
					if (Properties->TryGetNumberField("ReflectionsDelay", ReflectionsDelay))
					{
						ReverbEffect->ReflectionsDelay = Properties->GetNumberField("ReflectionsDelay");
					}
					if (Properties->TryGetNumberField("ReflectionsGain", ReflectionsGain))
					{
						ReverbEffect->ReflectionsGain = Properties->GetNumberField("ReflectionsGain");
					}
					if (Properties->TryGetNumberField("RoomRolloffFactor", RoomRolloffFactor))
					{
						ReverbEffect->RoomRolloffFactor = Properties->GetNumberField("RoomRolloffFactor");
					}
					if (Properties->TryGetNumberField("RoomRolloffFactor", RoomRolloffFactor))
					{
						ReverbEffect->RoomRolloffFactor = Properties->GetNumberField("RoomRolloffFactor");
					}
					if (Properties->TryGetBoolField("bChanged", bChanged))
					{
						ReverbEffect->bChanged = Properties->GetBoolField("bChanged");
					}
					if (Properties->TryGetBoolField("bBypassEarlyReflections", bBypassEarlyReflections))
					{
						ReverbEffect->bBypassEarlyReflections = Properties->GetBoolField("bBypassEarlyReflections");
					}
					if (Properties->TryGetBoolField("bBypassLateReflections", bBypassLateReflections))
					{
						ReverbEffect->bBypassLateReflections = Properties->GetBoolField("bBypassLateReflections");
					}

					FAssetRegistryModule::AssetCreated(ReverbEffect);
					ReverbEffect->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					ReverbEffect->PostEditChange();
					ReverbEffect->AddToRoot();
				}
				else if (Type == "MorphTarget")
				{
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					UPackage* Package = CreateAssetPackage(Name, OutFileNames);
					USkeleton* Skeleton = Cast<USkeleton>(GetSelectedAsset());

					USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(Skeleton, *Name, RF_Public | RF_Standalone);

					FAssetRegistryModule::AssetCreated(SkeletalMesh);
					SkeletalMesh->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					SkeletalMesh->PostEditChange();
					SkeletalMesh->AddToRoot();
				}
				else if (Type == "StaticMeshComponent")
				{
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					// Get World from Viewport
					UWorld* World = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport)->World();
					// Create Actor (StaticMeshActor)
					AStaticMeshActor* StaticMesh = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass());

					StaticMesh->SetMobility(EComponentMobility::Static);
					StaticMesh->SetActorLabel(DataObject->GetStringField("Outer"));

					// Set Transform
					const TSharedPtr<FJsonObject>* ActorLoc;

					if (Properties->TryGetObjectField("RelativeLocation", ActorLoc))
					{
						StaticMesh->SetActorLocation(ObjectToVector(ActorLoc->Get()));
					}

					const TSharedPtr<FJsonObject>* ActorRot;

					if (Properties->TryGetObjectField("RelativeRotation", ActorRot))
					{
						StaticMesh->SetActorRotation(ObjectToRotator(ActorRot->Get()));
					}

					const TSharedPtr<FJsonObject>* ActorScale;

					if (Properties->TryGetObjectField("RelativeScale3D", ActorScale))
					{
						StaticMesh->SetActorScale3D(ObjectToVector(ActorScale->Get()));
					}
					// ^ 
					// | completed

					const TSharedPtr<FJsonObject>* StaticMeshAs;

					if (Properties->TryGetObjectField("StaticMesh", StaticMeshAs))
					{
						FString MeshName;
						StaticMeshAs->Get()->GetStringField("ObjectName").Split(" ", nullptr, &MeshName);
						FString MeshPath;

						StaticMeshAs->Get()->GetStringField("ObjectPath").Replace(TEXT("FortniteGame/Content/"), TEXT("/Game/")).Split(
							".", &MeshPath, nullptr);

						UStaticMesh* StaticMeshAsset = LoadObject<UStaticMesh>(nullptr, *(MeshPath + "." + MeshName));

						UStaticMeshComponent* MeshComponent = StaticMesh->GetStaticMeshComponent();
						if (MeshComponent)
						{
							MeshComponent->SetStaticMesh(StaticMeshAsset);
							GLog->Log(MeshPath + "." + MeshName);
							GLog->Log(StaticMeshAsset->GetPathName());
						}
					}
				}
				else if (Type == "ParticleModuleTypeDataBeam2")
				{
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");
					TSharedPtr<FJsonObject> TaperScale = Properties->GetObjectField("TaperScale");
					TSharedPtr<FJsonObject> TableObject = TaperScale->GetObjectField("Table");

					// Create Lookup Table
					auto LookupTable = FDistributionLookupTable();

					LookupTable.Op = TableObject->GetIntegerField("Op");
					LookupTable.EntryCount = TableObject->GetIntegerField("EntryCount");
					LookupTable.EntryStride = TableObject->GetIntegerField("EntryStride");
					LookupTable.TimeScale = TableObject->GetNumberField("TimeScale");
					LookupTable.SubEntryStride = TableObject->GetNumberField("SubEntryStride");
					LookupTable.LockFlag = TableObject->GetNumberField("LockFlag");

					// Float Array
					auto Values = TableObject->GetArrayField("Values");

					for (int32 key_index = 0; key_index < Values.Num(); key_index++)
					{
						auto Value = Values[key_index]->AsNumber();

						LookupTable.Values.Add(Value);
					}

					GLog->Log("JsonAsAsset: Lookup Table GetValuesPerEntry: " + FString(*FString::SanitizeFloat(LookupTable.GetValuesPerEntry())));
					GLog->Log("JsonAsAsset: Lookup Table GetValueCount: " + FString(*FString::SanitizeFloat(LookupTable.GetValueCount())));

					FString UEA = FString("(");

					for (int x = 0; x < (LookupTable.GetValueCount() * 2); x += 1)
					{
						auto Time = x / 1000.0f;
						float OutValue = 0.1;

						const float* Entry1;
						const float* Entry2;
						float Alpha;

						LookupTable.GetEntry(Time, Entry1, Entry2, Alpha);
						OutValue = FMath::Lerp(Entry1[0], Entry2[0], Alpha);

						GLog->Log(
							"JsonAsAsset: Value: " + FString(*FString::SanitizeFloat(OutValue)) + "Time: " + FString(*FString::SanitizeFloat(Time)));

						UEA = UEA + FString(
							"(InVal=" + FString(*FString::SanitizeFloat(Time)) + ",OutVal=" + FString(*FString::SanitizeFloat(OutValue)) + "),");
					}

					UEA += ')';

					GLog->Log(UEA);
				}
				else if (Type == "SoundAttenuation")
				{
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					
					UPackage* Package = CreateAssetPackage(Name, OutFileNames);
					USoundAttenuation* SoundAttenuation = NewObject<USoundAttenuation>(Package, USoundAttenuation::StaticClass(), *Name,
					                                                                   EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
					TSharedPtr<FJsonObject> Attenuation = Properties->GetObjectField("Attenuation");

					bool bApplyNormalizationToStereoSounds;
					bool bAttenuate;
					bool bAttenuateWithLPF;
					bool bEnableFocusInterpolation;
					bool bEnableListenerFocus;
					bool bEnableLogFrequencyScaling;
					bool bEnableOcclusion;
					bool bEnablePriorityAttenuation;
					bool bEnableReverbSend;
					bool bEnableSubmixSends;
					bool bSpatialize;
					bool bUseComplexCollisionForOcclusion;

					if (Attenuation->TryGetBoolField("bApplyNormalizationToStereoSounds", bApplyNormalizationToStereoSounds))
					{
						SoundAttenuation->Attenuation.bApplyNormalizationToStereoSounds = Attenuation->GetBoolField(
							"bApplyNormalizationToStereoSounds");
					}
					if (Attenuation->TryGetBoolField("bAttenuate", bAttenuate))
					{
						SoundAttenuation->Attenuation.bAttenuate = Attenuation->GetBoolField("bAttenuate");
					}
					if (Attenuation->TryGetBoolField("bAttenuateWithLPF", bAttenuateWithLPF))
					{
						SoundAttenuation->Attenuation.bAttenuateWithLPF = Attenuation->GetBoolField("bAttenuateWithLPF");
					}
					if (Attenuation->TryGetBoolField("bEnableFocusInterpolation", bEnableFocusInterpolation))
					{
						SoundAttenuation->Attenuation.bEnableFocusInterpolation = Attenuation->GetBoolField("bEnableFocusInterpolation");
					}
					if (Attenuation->TryGetBoolField("bEnableListenerFocus", bEnableListenerFocus))
					{
						SoundAttenuation->Attenuation.bEnableListenerFocus = Attenuation->GetBoolField("bEnableListenerFocus");
					}
					if (Attenuation->TryGetBoolField("bEnableLogFrequencyScaling", bEnableLogFrequencyScaling))
					{
						SoundAttenuation->Attenuation.bEnableLogFrequencyScaling = Attenuation->GetBoolField("bEnableLogFrequencyScaling");
					}
					if (Attenuation->TryGetBoolField("bEnableOcclusion", bEnableOcclusion))
					{
						SoundAttenuation->Attenuation.bEnableOcclusion = Attenuation->GetBoolField("bEnableOcclusion");
					}
					if (Attenuation->TryGetBoolField("bEnablePriorityAttenuation", bEnablePriorityAttenuation))
					{
						SoundAttenuation->Attenuation.bEnablePriorityAttenuation = Attenuation->GetBoolField("bEnablePriorityAttenuation");
					}
					if (Attenuation->TryGetBoolField("bEnableReverbSend", bEnableReverbSend))
					{
						SoundAttenuation->Attenuation.bEnableReverbSend = Attenuation->GetBoolField("bEnableReverbSend");
					}
					if (Attenuation->TryGetBoolField("bEnableSubmixSends", bEnableSubmixSends))
					{
						SoundAttenuation->Attenuation.bEnableSubmixSends = Attenuation->GetBoolField("bEnableSubmixSends");
					}
					if (Attenuation->TryGetBoolField("bSpatialize", bSpatialize))
					{
						SoundAttenuation->Attenuation.bSpatialize = Attenuation->GetBoolField("bSpatialize");
					}
					if (Attenuation->TryGetBoolField("bUseComplexCollisionForOcclusion", bUseComplexCollisionForOcclusion))
					{
						SoundAttenuation->Attenuation.bUseComplexCollisionForOcclusion = Attenuation->
							GetBoolField("bUseComplexCollisionForOcclusion");
					}

					int64 BinauralRadius;
					int64 FocusAttackInterpSpeed;
					int64 FocusAzimuth;
					int64 FocusDistanceScale;
					int64 FocusPriorityScale;
					int64 FocusReleaseInterpSpeed;
					int64 FocusVolumeAttenuation;
					//
					int64 HPFFrequencyAtMax;
					int64 HPFFrequencyAtMin;
					int64 LPFRadiusMax;
					int64 LPFRadiusMin;
					//
					int64 ManualPriorityAttenuation;
					int64 ManualReverbSendLevel;
					int64 NonFocusAzimuth;
					int64 NonFocusDistanceScale;
					int64 NonFocusPriorityScale;
					int64 NonFocusVolumeAttenuation;
					int64 OcclusionInterpolationTime;
					int64 OcclusionLowPassFilterFrequency;
					int64 OcclusionVolumeAttenuation;
					int64 OmniRadius;
					int64 PriorityAttenuationDistanceMax;
					int64 PriorityAttenuationDistanceMin;
					int64 PriorityAttenuationMax;
					int64 PriorityAttenuationMin;
					int64 ReverbDistanceMax;
					int64 ReverbDistanceMin;
					int64 ReverbWetLevelMax;
					int64 ReverbWetLevelMin;
					int64 StereoSpread;
					int64 ConeOffset;
					int64 dBAttenuationAtMax;
					int64 FalloffDistance;

					if (Attenuation->TryGetNumberField("ConeOffset", ConeOffset))
					{
						SoundAttenuation->Attenuation.ConeOffset = Attenuation->GetNumberField("ConeOffset");
					}
					if (Attenuation->TryGetNumberField("dBAttenuationAtMax", dBAttenuationAtMax))
					{
						SoundAttenuation->Attenuation.dBAttenuationAtMax = Attenuation->GetNumberField("dBAttenuationAtMax");
					}
					if (Attenuation->TryGetNumberField("FalloffDistance", FalloffDistance))
					{
						SoundAttenuation->Attenuation.FalloffDistance = Attenuation->GetNumberField("FalloffDistance");
					}
					if (Attenuation->TryGetNumberField("BinauralRadius", BinauralRadius))
					{
						SoundAttenuation->Attenuation.BinauralRadius = Attenuation->GetNumberField("BinauralRadius");
					}
					if (Attenuation->TryGetNumberField("FocusAttackInterpSpeed", FocusAttackInterpSpeed))
					{
						SoundAttenuation->Attenuation.FocusAttackInterpSpeed = Attenuation->GetNumberField("FocusAttackInterpSpeed");
					}
					if (Attenuation->TryGetNumberField("FocusAzimuth", FocusAzimuth))
					{
						SoundAttenuation->Attenuation.FocusAzimuth = Attenuation->GetNumberField("FocusAzimuth");
					}
					if (Attenuation->TryGetNumberField("FocusDistanceScale", FocusDistanceScale))
					{
						SoundAttenuation->Attenuation.FocusDistanceScale = Attenuation->GetNumberField("FocusDistanceScale");
					}
					if (Attenuation->TryGetNumberField("FocusPriorityScale", FocusPriorityScale))
					{
						SoundAttenuation->Attenuation.FocusPriorityScale = Attenuation->GetNumberField("FocusPriorityScale");
					}
					if (Attenuation->TryGetNumberField("FocusReleaseInterpSpeed", FocusReleaseInterpSpeed))
					{
						SoundAttenuation->Attenuation.FocusReleaseInterpSpeed = Attenuation->GetNumberField("FocusReleaseInterpSpeed");
					}
					if (Attenuation->TryGetNumberField("FocusVolumeAttenuation", FocusVolumeAttenuation))
					{
						SoundAttenuation->Attenuation.FocusVolumeAttenuation = Attenuation->GetNumberField("FocusVolumeAttenuation");
					}
					if (Attenuation->TryGetNumberField("HPFFrequencyAtMax", HPFFrequencyAtMax))
					{
						SoundAttenuation->Attenuation.HPFFrequencyAtMax = Attenuation->GetNumberField("HPFFrequencyAtMax");
					}
					if (Attenuation->TryGetNumberField("HPFFrequencyAtMin", HPFFrequencyAtMin))
					{
						SoundAttenuation->Attenuation.HPFFrequencyAtMin = Attenuation->GetNumberField("HPFFrequencyAtMin");
					}
					if (Attenuation->TryGetNumberField("ManualPriorityAttenuation", ManualPriorityAttenuation))
					{
						SoundAttenuation->Attenuation.ManualPriorityAttenuation = Attenuation->GetNumberField("ManualPriorityAttenuation");
					}
					if (Attenuation->TryGetNumberField("LPFRadiusMin", LPFRadiusMin))
					{
						SoundAttenuation->Attenuation.LPFRadiusMin = Attenuation->GetNumberField("LPFRadiusMin");
					}
					if (Attenuation->TryGetNumberField("LPFRadiusMax", LPFRadiusMax))
					{
						SoundAttenuation->Attenuation.LPFRadiusMax = Attenuation->GetNumberField("LPFRadiusMax");
					}
					if (Attenuation->TryGetNumberField("ManualReverbSendLevel", ManualReverbSendLevel))
					{
						SoundAttenuation->Attenuation.ManualReverbSendLevel = Attenuation->GetNumberField("ManualReverbSendLevel");
					}
					if (Attenuation->TryGetNumberField("NonFocusAzimuth", NonFocusAzimuth))
					{
						SoundAttenuation->Attenuation.NonFocusAzimuth = Attenuation->GetNumberField("NonFocusAzimuth");
					}
					if (Attenuation->TryGetNumberField("NonFocusDistanceScale", NonFocusDistanceScale))
					{
						SoundAttenuation->Attenuation.NonFocusDistanceScale = Attenuation->GetNumberField("NonFocusDistanceScale");
					}
					if (Attenuation->TryGetNumberField("NonFocusPriorityScale", NonFocusPriorityScale))
					{
						SoundAttenuation->Attenuation.NonFocusPriorityScale = Attenuation->GetNumberField("NonFocusPriorityScale");
					}
					if (Attenuation->TryGetNumberField("NonFocusVolumeAttenuation", NonFocusVolumeAttenuation))
					{
						SoundAttenuation->Attenuation.NonFocusVolumeAttenuation = Attenuation->GetNumberField("NonFocusVolumeAttenuation");
					}
					if (Attenuation->TryGetNumberField("OcclusionInterpolationTime", OcclusionInterpolationTime))
					{
						SoundAttenuation->Attenuation.OcclusionInterpolationTime = Attenuation->GetNumberField("OcclusionInterpolationTime");
					}
					if (Attenuation->TryGetNumberField("OcclusionLowPassFilterFrequency", OcclusionLowPassFilterFrequency))
					{
						SoundAttenuation->Attenuation.OcclusionLowPassFilterFrequency = Attenuation->
							GetNumberField("OcclusionLowPassFilterFrequency");
					}
					if (Attenuation->TryGetNumberField("OcclusionVolumeAttenuation", OcclusionVolumeAttenuation))
					{
						SoundAttenuation->Attenuation.OcclusionVolumeAttenuation = Attenuation->GetNumberField("OcclusionVolumeAttenuation");
					}
					if (Attenuation->TryGetNumberField("OmniRadius", OmniRadius))
					{
						SoundAttenuation->Attenuation.OmniRadius = Attenuation->GetNumberField("OmniRadius");
					}
					if (Attenuation->TryGetNumberField("PriorityAttenuationDistanceMax", PriorityAttenuationDistanceMax))
					{
						SoundAttenuation->Attenuation.PriorityAttenuationDistanceMax = Attenuation->GetNumberField("PriorityAttenuationDistanceMax");
					}
					if (Attenuation->TryGetNumberField("PriorityAttenuationDistanceMin", PriorityAttenuationDistanceMin))
					{
						SoundAttenuation->Attenuation.PriorityAttenuationDistanceMin = Attenuation->GetNumberField("PriorityAttenuationDistanceMin");
					}
					if (Attenuation->TryGetNumberField("PriorityAttenuationMax", PriorityAttenuationMax))
					{
						SoundAttenuation->Attenuation.PriorityAttenuationMax = Attenuation->GetNumberField("PriorityAttenuationMax");
					}
					if (Attenuation->TryGetNumberField("PriorityAttenuationMin", PriorityAttenuationMin))
					{
						SoundAttenuation->Attenuation.PriorityAttenuationMin = Attenuation->GetNumberField("PriorityAttenuationMin");
					}
					if (Attenuation->TryGetNumberField("ReverbDistanceMax", ReverbDistanceMax))
					{
						SoundAttenuation->Attenuation.ReverbDistanceMax = Attenuation->GetNumberField("ReverbDistanceMax");
					}
					if (Attenuation->TryGetNumberField("ReverbDistanceMin", ReverbDistanceMin))
					{
						SoundAttenuation->Attenuation.ReverbDistanceMin = Attenuation->GetNumberField("ReverbDistanceMin");
					}
					if (Attenuation->TryGetNumberField("ReverbWetLevelMax", ReverbWetLevelMax))
					{
						SoundAttenuation->Attenuation.ReverbWetLevelMax = Attenuation->GetNumberField("ReverbWetLevelMax");
					}
					if (Attenuation->TryGetNumberField("ReverbWetLevelMin", ReverbWetLevelMin))
					{
						SoundAttenuation->Attenuation.ReverbWetLevelMin = Attenuation->GetNumberField("ReverbWetLevelMin");
					}
					if (Attenuation->TryGetNumberField("StereoSpread", StereoSpread))
					{
						SoundAttenuation->Attenuation.StereoSpread = Attenuation->GetNumberField("StereoSpread");
					}

					FString AbsorptionMethod;
					FString OcclusionTraceChannel;
					FString PriorityAttenuationMethod;
					FString ReverbSendMethod;
					FString SpatializationAlgorithm;
					FString AttenuationShape;
					FString DistanceAlgorithm;
					FString FalloffMode;

					if (Attenuation->TryGetStringField("AbsorptionMethod", AbsorptionMethod))
					{
						SoundAttenuation->Attenuation.AbsorptionMethod =
							AbsorptionMethod.EndsWith("CustomCurve") ? EAirAbsorptionMethod::CustomCurve : EAirAbsorptionMethod::Linear;
					}
					if (Attenuation->TryGetStringField("PriorityAttenuationMethod", PriorityAttenuationMethod))
					{
						SoundAttenuation->Attenuation.PriorityAttenuationMethod =
							PriorityAttenuationMethod.EndsWith("CustomCurve")
								? EPriorityAttenuationMethod::CustomCurve
								: PriorityAttenuationMethod.EndsWith("Manual")
								? EPriorityAttenuationMethod::Manual
								: EPriorityAttenuationMethod::Linear;
					}
					if (Attenuation->TryGetStringField("ReverbSendMethod", ReverbSendMethod))
					{
						SoundAttenuation->Attenuation.ReverbSendMethod =
							ReverbSendMethod.EndsWith("CustomCurve")
								? EReverbSendMethod::CustomCurve
								: ReverbSendMethod.EndsWith("Manual")
								? EReverbSendMethod::Manual
								: EReverbSendMethod::Linear;
					}
					if (Attenuation->TryGetStringField("FalloffMode", FalloffMode))
					{
						SoundAttenuation->Attenuation.FalloffMode =
							FalloffMode.EndsWith("Hold")
								? ENaturalSoundFalloffMode::Hold
								: FalloffMode.EndsWith("Silent")
								? ENaturalSoundFalloffMode::Silent
								: ENaturalSoundFalloffMode::Continues;
					}
					if (Attenuation->TryGetStringField("DistanceAlgorithm", DistanceAlgorithm))
					{
						SoundAttenuation->Attenuation.DistanceAlgorithm =
							DistanceAlgorithm.EndsWith("Logarithmic")
								? EAttenuationDistanceModel::Logarithmic
								: DistanceAlgorithm.EndsWith("Inverse")
								? EAttenuationDistanceModel::Inverse
								: DistanceAlgorithm.EndsWith("LogReverse")
								? EAttenuationDistanceModel::LogReverse
								: DistanceAlgorithm.EndsWith("NaturalSound")
								? EAttenuationDistanceModel::NaturalSound
								: DistanceAlgorithm.EndsWith("Custom")
								? EAttenuationDistanceModel::Custom
								: EAttenuationDistanceModel::Linear;
					}
					if (Attenuation->TryGetStringField("SpatializationAlgorithm", SpatializationAlgorithm))
					{
						SoundAttenuation->Attenuation.SpatializationAlgorithm =
							SpatializationAlgorithm.EndsWith("SPATIALIZATION_HRTF")
								? ESoundSpatializationAlgorithm::SPATIALIZATION_HRTF
								: ESoundSpatializationAlgorithm::SPATIALIZATION_Default;
					}
					if (Attenuation->TryGetStringField("AttenuationShape", AttenuationShape))
					{
						SoundAttenuation->Attenuation.AttenuationShape =
							AttenuationShape.EndsWith("Capsule")
								? EAttenuationShape::Type::Capsule
								: AttenuationShape.EndsWith("Box")
								? EAttenuationShape::Type::Box
								: AttenuationShape.EndsWith("Cone")
								? EAttenuationShape::Type::Cone
								: EAttenuationShape::Type::Sphere;
					}
					if (Attenuation->TryGetStringField("OcclusionTraceChannel", OcclusionTraceChannel))
					{
						SoundAttenuation->Attenuation.OcclusionTraceChannel =
							OcclusionTraceChannel.EndsWith("ECC_WorldStatic")
								? ECollisionChannel::ECC_WorldStatic
								: OcclusionTraceChannel.EndsWith("ECC_WorldDynamic")
								? ECollisionChannel::ECC_WorldDynamic
								: OcclusionTraceChannel.EndsWith("ECC_Pawn")
								? ECollisionChannel::ECC_Pawn
								: OcclusionTraceChannel.EndsWith("ECC_Visibility")
								? ECollisionChannel::ECC_Visibility
								: OcclusionTraceChannel.EndsWith("ECC_Camera")
								? ECollisionChannel::ECC_Camera
								: OcclusionTraceChannel.EndsWith("ECC_PhysicsBody")
								? ECollisionChannel::ECC_PhysicsBody
								: OcclusionTraceChannel.EndsWith("ECC_Vehicle")
								? ECollisionChannel::ECC_Vehicle
								: OcclusionTraceChannel.EndsWith("ECC_Destructible")
								? ECollisionChannel::ECC_Destructible
								: OcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel1")
								? ECollisionChannel::ECC_EngineTraceChannel1
								: OcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel2")
								? ECollisionChannel::ECC_EngineTraceChannel2
								: OcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel3")
								? ECollisionChannel::ECC_EngineTraceChannel3
								: OcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel4")
								? ECollisionChannel::ECC_EngineTraceChannel4
								: OcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel5")
								? ECollisionChannel::ECC_EngineTraceChannel5
								: OcclusionTraceChannel.EndsWith("ECC_EngineTraceChannel6")
								? ECollisionChannel::ECC_EngineTraceChannel6
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel1")
								? ECollisionChannel::ECC_GameTraceChannel1
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel2")
								? ECollisionChannel::ECC_GameTraceChannel2
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel3")
								? ECollisionChannel::ECC_GameTraceChannel3
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel4")
								? ECollisionChannel::ECC_GameTraceChannel4
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel5")
								? ECollisionChannel::ECC_GameTraceChannel5
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel6")
								? ECollisionChannel::ECC_GameTraceChannel6
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel7")
								? ECollisionChannel::ECC_GameTraceChannel7
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel8")
								? ECollisionChannel::ECC_GameTraceChannel8
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel9")
								? ECollisionChannel::ECC_GameTraceChannel9
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel10")
								? ECollisionChannel::ECC_GameTraceChannel10
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel11")
								? ECollisionChannel::ECC_GameTraceChannel11
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel12")
								? ECollisionChannel::ECC_GameTraceChannel12
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel13")
								? ECollisionChannel::ECC_GameTraceChannel13
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel14")
								? ECollisionChannel::ECC_GameTraceChannel14
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel15")
								? ECollisionChannel::ECC_GameTraceChannel15
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel16")
								? ECollisionChannel::ECC_GameTraceChannel16
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel17")
								? ECollisionChannel::ECC_GameTraceChannel17
								: OcclusionTraceChannel.EndsWith("ECC_GameTraceChannel18")
								? ECollisionChannel::ECC_GameTraceChannel18
								: OcclusionTraceChannel.EndsWith("ECC_OverlapAll_Deprecated")
								? ECollisionChannel::ECC_OverlapAll_Deprecated
								: ECollisionChannel::ECC_Visibility;
					}

					const TSharedPtr<FJsonObject>* AttenuationShapeExtents;

					if (Attenuation->TryGetObjectField("AttenuationShapeExtents", AttenuationShapeExtents))
					{
						SoundAttenuation->Attenuation.AttenuationShapeExtents = ObjectToVector(AttenuationShapeExtents->Get());
					}

					const TSharedPtr<FJsonObject>* CustomAttenuationCurve;

					if (Attenuation->TryGetObjectField("CustomAttenuationCurve", CustomAttenuationCurve))
					{
						TArray<TSharedPtr<FJsonValue>> Keys = Attenuation->GetObjectField("CustomAttenuationCurve")->GetObjectField("EditorCurveData")
						                                                 ->GetArrayField("Keys");
						FRichCurve Curve;

						for (int32 key_index = 0; key_index < Keys.Num(); key_index++)
						{
							// RCIM_Cubic
							TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();
							ERichCurveInterpMode InterpMode =
								Key->GetStringField("InterpMode") == "RCIM_Linear"
									? ERichCurveInterpMode::RCIM_Linear
									: // Linear
									Key->GetStringField("InterpMode") == "RCIM_Cubic"
									? ERichCurveInterpMode::RCIM_Cubic
									: // Cubic
									Key->GetStringField("InterpMode") == "RCIM_Constant"
									? ERichCurveInterpMode::RCIM_Constant
									: // Constant
									ERichCurveInterpMode::RCIM_None;

							FRichCurveKey RichKey = FRichCurveKey(float(Key->GetNumberField("Time")), float(Key->GetNumberField("Value")),
							                                      float(Key->GetNumberField("ArriveTangent")),
							                                      float(Key->GetNumberField("LeaveTangent")), InterpMode);

							Curve.Keys.Add(RichKey);
						}

						SoundAttenuation->Attenuation.CustomAttenuationCurve.EditorCurveData = Curve;
					}

					FAssetRegistryModule::AssetCreated(SoundAttenuation);
					SoundAttenuation->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					SoundAttenuation->PostEditChange();
					SoundAttenuation->AddToRoot();
				}
			}
		}
	}
}

void FJsonAsAssetModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FJsonAsAssetCommands::Get().PluginAction, PluginCommands);
		}
	}
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FJsonAsAssetCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

FVector FJsonAsAssetModule::ObjectToVector(FJsonObject* Object)
{
	return FVector(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"));
}

FRotator FJsonAsAssetModule::ObjectToRotator(FJsonObject* Object)
{
	return FRotator(Object->GetNumberField("Pitch"), Object->GetNumberField("Yaw"), Object->GetNumberField("Roll"));
}

FQuat FJsonAsAssetModule::ObjectToQuat(FJsonObject* Object)
{
	return FQuat(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"), Object->GetNumberField("W"));
}

FLinearColor FJsonAsAssetModule::ObjectToLinearColor(FJsonObject* Object)
{
	return FLinearColor(Object->GetNumberField("R"), Object->GetNumberField("G"), Object->GetNumberField("B"), Object->GetNumberField("A"));
}

bool FJsonAsAssetModule::EvaluateAnimSequence(FJsonObject* Object, UAnimSequenceBase* AnimSequenceBase)
{
	// Properties of the object
	TSharedPtr<FJsonObject> Properties = Object->GetObjectField("Properties");

	TArray<TSharedPtr<FJsonValue>> FloatCurves;
	TArray<TSharedPtr<FJsonValue>> Notifies;
	TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers;
	const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1;
	const TArray<TSharedPtr<FJsonValue>>* Notifiesa;
	const TSharedPtr<FJsonObject>* RawCurveData;

	if (Properties->TryGetObjectField("RawCurveData", RawCurveData))
	{
		FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
	}
	else
	{
		if (Object->TryGetObjectField("CompressedCurveData", RawCurveData))
		{
			FloatCurves = Object->GetObjectField("CompressedCurveData")->GetArrayField("FloatCurves");
		}
	}

	for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves)
	{
		TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");

		GLog->Log("JsonAsAsset: Added animation curve: " + FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName"));

		USkeleton* Skeleton = AnimSequenceBase->GetSkeleton();
		FSmartName NewTrackName;

		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName,
		                                FName(*FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName")), NewTrackName);

		int CurveTypeFlags = FloatCurveObject->AsObject()->GetIntegerField("CurveTypeFlags");

		ensureAlways(Skeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, NewTrackName.UID, NewTrackName));
		// AnimSequenceBase->RawCurveData.AddCurveData(NewTrackName, CurveTypeFlags);

		for (int32 key_index = 0; key_index < Keys.Num(); key_index++)
		{
			TSharedPtr<FJsonObject> Key = Keys[key_index]->AsObject();
			ERichCurveInterpMode InterpMode =
				Key->GetStringField("InterpMode") == "RCIM_Linear"
					? ERichCurveInterpMode::RCIM_Linear
					: // Linear
					Key->GetStringField("InterpMode") == "RCIM_Cubic"
					? ERichCurveInterpMode::RCIM_Cubic
					: // Cubic
					Key->GetStringField("InterpMode") == "RCIM_Constant"
					? ERichCurveInterpMode::RCIM_Constant
					: // Constant
					ERichCurveInterpMode::RCIM_None;

			FRichCurveKey RichKey = FRichCurveKey(float(Key->GetNumberField("Time")), float(Key->GetNumberField("Value")),
			                                      float(Key->GetNumberField("ArriveTangent")), float(Key->GetNumberField("LeaveTangent")),
			                                      InterpMode);

			// AnimSequenceBase->RawCurveData.AddFloatCurveKey(NewTrackName, CurveTypeFlags, float(Key->GetNumberField("Time")),
			//                                                 float(Key->GetNumberField("Value")));
			// AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().ArriveTangent = float(Key->GetNumberField("ArriveTangent"));
			// AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().LeaveTangent = float(Key->GetNumberField("LeaveTangent"));
			// AnimSequenceBase->RawCurveData.FloatCurves.Last().FloatCurve.Keys.Last().InterpMode = InterpMode;
		}
	}

	if (Properties->TryGetArrayField("Notifies", Notifiesa))
	{
		Notifies = Properties->GetArrayField("Notifies");

		for (TSharedPtr<FJsonValue> Notify : Notifies)
		{
		}
	}

	UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);

	if (Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence)
	{
		AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

		for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers)
		{
			FAnimSyncMarker AuthoredSyncMarker = FAnimSyncMarker();
			AuthoredSyncMarker.MarkerName = FName(*SyncMarker.Get()->AsObject().Get()->GetStringField("MarkerName"));
			AuthoredSyncMarker.Time = SyncMarker.Get()->AsObject().Get()->GetNumberField("Time");
			CastedAnimSequence->AuthoredSyncMarkers.Add(AuthoredSyncMarker);
		}
	}


	// if (CastedAnimSequence)
	// {
	// 	CastedAnimSequence->MarkRawDataAsModified();
	// 	CastedAnimSequence->OnRawDataChanged();
	// 	CastedAnimSequence->RequestSyncAnimRecompression();
	// }
	//
	// UAnimMontage* CastedAnimMontage = Cast<UAnimMontage>(AnimSequenceBase);
	//
	// AnimSequenceBase->MarkRawDataAsModified();
	// AnimSequenceBase->Modify();
	// AnimSequenceBase->RefreshCurveData();
	// AnimSequenceBase->PostEditChange();

	return true;
}

UObject* FJsonAsAssetModule::GetSelectedAsset()
{
	// The content browser has a module, which helps us extract the
	// selected assets, as it is completely connected.
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.Num() == 0)
	{
		GLog->Log("JsonAsAsset: [GetSelectedAsset] None selected, returning nullptr.");

		FText DialogText = FText::FromString(TEXT("A function to find a selected asset failed, please select a asset to go further."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		// None found, therefore we need to return nullptr.
		return nullptr;
	}

	// Return only the first selected asset
	return SelectedAssets[0].GetAsset();
}

UPackage* FJsonAsAssetModule::CreateAssetPackage(const FString& Name, const TArray<FString>& Files) const
{
	UPackage* Ignore = nullptr;
	return CreateAssetPackage(Name, Files, Ignore);
}

UPackage* FJsonAsAssetModule::CreateAssetPackage(const FString& Name, const TArray<FString>& Files, UPackage*& OutOutermostPkg) const
{
	// TODO: Support virtual paths (plugins)
	const FString OutputPath = Files[0];
	FString Path;

	OutputPath.Split("FortniteGame/Content/", nullptr, &Path);
	Path.Split("/", &Path, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	const FString PathWithGame = "/Game/" + Path + "/" + Name;
	UPackage* Package = CreatePackage(*PathWithGame);
	OutOutermostPkg = Package->GetOutermost();
	Package->FullyLoad();

	return Package;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)
