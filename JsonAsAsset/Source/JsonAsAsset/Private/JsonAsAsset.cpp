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

#include "Misc/MessageDialog.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "ToolMenus.h"

// | ------------------------------------------------------

// ----> Asset Classes
#include "Animation/MorphTarget.h"
#include "Animation/AnimSequence.h"
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
#include "Sound/ReverbEffect.h"

// ----> Factories
#include "Importers/CurveFloatImporter.h"
#include "Importers/CurveLinearColorImporter.h"
#include "Importers/DataTableImporter.h"
#include "Importers/Importer.h"
#include "Importers/ParticleModuleTypeDataBeam2Importer.h"
#include "Importers/SkeletalMeshLODSettingsImporter.h"
#include "Importers/SkeletonImporter.h"
#include "Importers/SoundAttenuationImporter.h"
#include "Importers/SubsurfaceProfileImporter.h"

// ------------------------------------------------------ |

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

void FJsonAsAssetModule::StartupModule() {
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

void FJsonAsAssetModule::ShutdownModule() {
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FJsonAsAssetStyle::Shutdown();

	FJsonAsAssetCommands::Unregister();
}

void FJsonAsAssetModule::PluginButtonClicked() {
	TArray<FString> OutFileNames;

	void* ParentWindowHandle = nullptr;

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();

	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) {
		ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		uint32 SelectionFlag = 0;
		DesktopPlatform->OpenFileDialog(ParentWindowHandle, TEXT("Open a JSON file"), TEXT(""), FString(""), TEXT("JSON Files|*.json"), SelectionFlag,
		                                OutFileNames);
	}

	if (OutFileNames.Num() != 0 && OutFileNames[0] != "") {
		FString ContentBefore;
		FFileHelper::LoadFileToString(ContentBefore, *OutFileNames[0]);

		// For some reason a array
		// without it being in a object.
		FString Content = FString(TEXT("{\"data\": "));
		Content.Append(ContentBefore);
		Content.Append(FString("}"));

		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
			GLog->Log("JsonAsAsset: Deserialized file, reading the contents.");

			TArray<FString> Types;
			TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField("data");
			for (TSharedPtr<FJsonValue>& Obj : DataObjects) {
				Types.Add(Obj->AsObject()->GetStringField("Type"));
			}

			if (!IImporter::CanImportAny(Types)) {
				FText DialogText = FText::FromString("No exports from \"" + OutFileNames[0] + "\" can be imported!");
				FMessageDialog::Open(EAppMsgType::Ok, DialogText);
			}

			for (TSharedPtr<FJsonValue>& Obj : DataObjects) {
				TSharedPtr<FJsonObject> DataObject = Obj->AsObject();

				FString Type = DataObject->GetStringField("Type");
				FString Name = DataObject->GetStringField("Name");

				if (IImporter::CanImport(Type)) {
					UPackage* OutermostPkg;
					UPackage* Package = CreateAssetPackage(Name, OutFileNames, OutermostPkg);
					IImporter* Importer;

					// Curves
					if (Type == "CurveFloat") Importer = new UCurveFloatImporter(Name, DataObject, Package, OutermostPkg);
					// else if (Type == "CurveVector") Importer = new UCurveVectorImporter(Name, DataObject, Package, OutermostPkg); // TODO
					else if (Type == "CurveLinearColor") Importer = new UCurveLinearColorImporter(Name, DataObject, Package, OutermostPkg);

					// Skeletal
					else if (Type == "SkeletalMeshLODSettings") Importer = new USkeletalMeshLODSettingsImporter(Name, DataObject, Package, OutermostPkg);
					else if (Type == "Skeleton") Importer = new USkeletonImporter(Name, DataObject, Package, OutermostPkg, DataObjects);

					// Other
					else if (Type == "DataTable") Importer = new UDataTableImporter(Name, DataObject, Package, OutermostPkg);
					else if (Type == "SoundAttenuation") Importer = new USoundAttenuationImporter(Name, DataObject, Package, OutermostPkg);
					else if (Type == "SubsurfaceProfile") Importer = new USubsurfaceProfileImporter(Name, DataObject, Package, OutermostPkg);
					else if (Type == "ParticleModuleTypeDataBeam2") Importer = new UParticleModuleTypeDataBeam2Importer(Name, DataObject, Package, OutermostPkg);
					else Importer = nullptr;

					if (Importer != nullptr && Importer->ImportData()) {
						UE_LOG(LogJson, Log, TEXT("Successfully imported \"%s\" as \"%s\""), *Name, *Type)
					} else {
						FText DialogText = FText::FromString("The \"" + Type + "\" cannot be imported!");
						FMessageDialog::Open(EAppMsgType::Ok, DialogText);
					}
				}

				if (Type == "PhysicsAsset") {
					GLog->Log("JsonAsAsset: Found a physics asset by the name of " + Name + ", parsing..");

					// We cannot create a skeletal mesh at the moment
					// so for now we just add to the skeletal mesh, which
					// is good enough for now.
					FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
					TArray<FAssetData> SelectedAssets;
					ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

					// Properties of the object
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					for (FAssetData& AssetData : SelectedAssets) {
						UObject* Asset = AssetData.GetAsset();

						if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset)) {
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

							for (TSharedPtr<FJsonValue> SecondaryPurposeValueObject : DataObjects) {
								TSharedPtr<FJsonObject> SecondaryPurposeObject = SecondaryPurposeValueObject->AsObject();

								FString SecondaryPurposeType = SecondaryPurposeObject->GetStringField("Type");
								FString SecondaryPurposeName = SecondaryPurposeObject->GetStringField("Name");

								int32 anIndex = 0;

								// Import all sockets
								if (SecondaryPurposeType == "SkeletalBodySetup") {
									// Properties of the object
									TSharedPtr<FJsonObject> SecondaryPurposeProperties = SecondaryPurposeObject->GetObjectField("Properties");
									USkeletalBodySetup* NewBodySetup = NewObject<USkeletalBodySetup>(PhysAsset, NAME_None, RF_Transactional);

									NewBodySetup->BodySetupGuid = FGuid(*SecondaryPurposeObject->GetStringField("BodySetupGuid"));
									NewBodySetup->BoneName = FName(*SecondaryPurposeProperties->GetStringField("BoneName"));

									FString PhysicsType;
									FString CollisionTraceFlag;
									FString CollisionReponse;

									if (SecondaryPurposeProperties->TryGetStringField("PhysicsType", PhysicsType)) {
										NewBodySetup->PhysicsType =
											PhysicsType.EndsWith("PhysType_Default")
												? EPhysicsType::PhysType_Default
												: PhysicsType.EndsWith("PhysType_Kinematic")
												? EPhysicsType::PhysType_Kinematic
												: PhysicsType.EndsWith("PhysType_Simulated")
												? EPhysicsType::PhysType_Simulated
												: EPhysicsType::PhysType_Default;
									}
									if (SecondaryPurposeProperties->TryGetStringField("CollisionReponse", CollisionReponse)) {
										NewBodySetup->CollisionReponse =
											CollisionReponse.EndsWith("BodyCollision_Enabled")
												? EBodyCollisionResponse::BodyCollision_Enabled
												: CollisionReponse.EndsWith("BodyCollision_Disabled")
												? EBodyCollisionResponse::BodyCollision_Disabled
												: EBodyCollisionResponse::BodyCollision_Enabled;
									}
									if (SecondaryPurposeProperties->TryGetStringField("CollisionTraceFlag", CollisionTraceFlag)) {
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
									if (SecondaryPurposeProperties->TryGetObjectField("AggGeom", AggGeom)) {
										const TArray<TSharedPtr<FJsonValue>>* SphylElems;
										FKAggregateGeom AggOem;

										if (AggGeom->Get()->TryGetArrayField("SphylElems", SphylElems)) {
											for (TSharedPtr<FJsonValue> SphylElemOb : AggGeom->Get()->GetArrayField("SphylElems")) {
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

										if (AggGeom->Get()->TryGetArrayField("SphereElems", SphylElems)) {
											for (TSharedPtr<FJsonValue> SphylElemOb : AggGeom->Get()->GetArrayField("SphereElems")) {
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

										if (AggGeom->Get()->TryGetArrayField("BoxElems", SphylElems)) {
											for (TSharedPtr<FJsonValue> SphylElemOb : AggGeom->Get()->GetArrayField("BoxElems")) {
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

										if (AggGeom->Get()->TryGetArrayField("TaperedCapsuleElems", SphylElems)) {
											for (TSharedPtr<FJsonValue> TaperedCapsuleElemOb : AggGeom->Get()->GetArrayField("TaperedCapsuleElems")) {
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
									if (SecondaryPurposeProperties->TryGetObjectField("DefaultInstance", DefaultInstance)) {
										FBodyInstance BodyInstance;

										int64 MassInKgOverride;
										int64 AngularDamping;
										int64 LinearDamping;
										int64 PositionSolverIterationCount;
										int64 VelocitySolverIterationCount;
										const TSharedPtr<FJsonObject>* InertiaTensorScale;
										const TSharedPtr<FJsonObject>* COMNudge;

										if (DefaultInstance->Get()->TryGetNumberField("MassInKgOverride", MassInKgOverride)) {
											BodyInstance.SetMassOverride(DefaultInstance->Get()->GetNumberField("MassInKgOverride"), true);
										}
										if (DefaultInstance->Get()->TryGetNumberField("AngularDamping", AngularDamping)) {
											BodyInstance.AngularDamping = DefaultInstance->Get()->GetNumberField("AngularDamping");
										}
										if (DefaultInstance->Get()->TryGetNumberField("LinearDamping", LinearDamping)) {
											BodyInstance.LinearDamping = DefaultInstance->Get()->GetNumberField("LinearDamping");
										}
										if (DefaultInstance->Get()->TryGetNumberField("PositionSolverIterationCount", PositionSolverIterationCount)) {
											BodyInstance.PositionSolverIterationCount = DefaultInstance->Get()->GetIntegerField(
												"PositionSolverIterationCount");
										}
										if (DefaultInstance->Get()->TryGetNumberField("VelocitySolverIterationCount", VelocitySolverIterationCount)) {
											int Velo = DefaultInstance->Get()->GetIntegerField("VelocitySolverIterationCount");
											BodyInstance.VelocitySolverIterationCount = *((uint8*)&Velo);
										}
										if (DefaultInstance->Get()->TryGetObjectField("InertiaTensorScale", InertiaTensorScale)) {
											BodyInstance.InertiaTensorScale = ObjectToVector(InertiaTensorScale->Get());
										}
										if (DefaultInstance->Get()->TryGetObjectField("COMNudge", COMNudge)) {
											BodyInstance.COMNudge = ObjectToVector(COMNudge->Get());
										}
									}

									int32 BodySetupIndex = PhysAsset->SkeletalBodySetups.Add(NewBodySetup);

									PhysAsset->UpdateBodySetupIndexMap();
									PhysAsset->UpdateBoundsBodiesArray();
								}
								if (SecondaryPurposeType == "PhysicsConstraintTemplate") {
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
										"ProfileInstance", ProfileInstance11)) {
										const FJsonObject* ProfileInstance = ProfileInstance11->Get();

										const TSharedPtr<FJsonObject>* LinearLimit;

										if (ProfileInstance->TryGetObjectField("LinearLimit", LinearLimit)) {
											bool bSoftConstraint;
											int64 ContactDistance;
											int64 Damping;
											int64 Restitution;
											int64 Stiffness;
											int64 Limit;

											if (LinearLimit->Get()->TryGetBoolField("bSoftConstraint", bSoftConstraint)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.bSoftConstraint = bSoftConstraint;
											}

											if (LinearLimit->Get()->TryGetNumberField("ContactDistance", ContactDistance)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.ContactDistance = LinearLimit->Get()->
												                                                                                               GetNumberField("ContactDistance");
											}

											if (LinearLimit->Get()->TryGetNumberField("Damping", Damping)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Damping = LinearLimit->Get()->
												                                                                                       GetNumberField("Damping");
											}

											if (LinearLimit->Get()->TryGetNumberField("Restitution", Restitution)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Restitution = LinearLimit->Get()->
												                                                                                           GetNumberField("Restitution");
											}

											if (LinearLimit->Get()->TryGetNumberField("Stiffness", Stiffness)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.LinearLimit.Stiffness = LinearLimit->Get()->
												                                                                                         GetNumberField("Stiffness");
											}

											if (LinearLimit->Get()->TryGetNumberField("Limit", Limit)) {
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
										if (ProfileInstance->TryGetObjectField("ConeLimit", ConeLimit)) {
											int64 Swing1LimitDegrees;
											int64 Swing2LimitDegrees;
											FString Swing1Motion;
											FString Swing2Motion;

											if (ConeLimit->Get()->TryGetNumberField("Swing1LimitDegrees", Swing1LimitDegrees)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing1LimitDegrees = ConeLimit->Get()->
												                                                                                              GetNumberField("Swing1LimitDegrees");
											}
											if (ConeLimit->Get()->TryGetNumberField("Swing2LimitDegrees", Swing2LimitDegrees)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing2LimitDegrees = ConeLimit->Get()->
												                                                                                              GetNumberField("Swing2LimitDegrees");
											}

											if (ConeLimit->Get()->TryGetStringField("Swing1Motion", Swing1Motion)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.ConeLimit.Swing1Motion =
													ConeLimit->Get()->GetStringField("Swing1Motion").EndsWith("ACM_Free")
														? EAngularConstraintMotion::ACM_Free
														: ConeLimit->Get()->GetStringField("Swing1Motion").EndsWith("ACM_Limited")
														? EAngularConstraintMotion::ACM_Limited
														: ConeLimit->Get()->GetStringField("Swing1Motion").EndsWith("ACM_Locked")
														? EAngularConstraintMotion::ACM_Locked
														: EAngularConstraintMotion::ACM_Limited;
											}
											if (ConeLimit->Get()->TryGetStringField("Swing2Motion", Swing2Motion)) {
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
										if (ProfileInstance->TryGetObjectField("TwistLimit", TwistLimit)) {
											int64 TwistLimitDegrees;
											FString TwistMotion;

											if (TwistLimit->Get()->TryGetNumberField("TwistLimitDegrees", TwistLimitDegrees)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.TwistLimit.TwistLimitDegrees = TwistLimit->Get()->
												                                                                                               GetNumberField("TwistLimitDegrees");
											}

											if (TwistLimit->Get()->TryGetStringField("TwistMotion", TwistMotion)) {
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
										if (ProfileInstance->TryGetObjectField("AngularDrive", AngularDrive)) {
											const TSharedPtr<FJsonObject>* TwistDrive;

											const TSharedPtr<FJsonObject>* SlerpDrive;

											if (AngularDrive->Get()->TryGetObjectField("SlerpDrive", SlerpDrive)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.SlerpDrive.bEnablePositionDrive =
													SlerpDrive->Get()->GetBoolField("bEnablePositionDrive");
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.SlerpDrive.Stiffness = SlerpDrive->
												                                                                                        Get()->GetNumberField("Stiffness");
											}

											if (AngularDrive->Get()->TryGetObjectField("TwistDrive", TwistDrive)) {
												NewConstraintSetup->DefaultInstance.ProfileInstance.AngularDrive.TwistDrive.Stiffness = TwistDrive->
												                                                                                        Get()->GetNumberField("Stiffness");
											}

											const TSharedPtr<FJsonObject>* SwingDrive;

											if (AngularDrive->Get()->TryGetObjectField("SwingDrive", SwingDrive)) {
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
						else {
							FText DialogText = FText::FromString(TEXT("Please select a skeletal mesh to modify using the plugin."));
							FMessageDialog::Open(EAppMsgType::Ok, DialogText);
						}
					}

					// Nothing Selected At All
					if (SelectedAssets.Num() == 0) {
						FText DialogText = FText::FromString(TEXT(
							"You have no selected asset, please select a Skeletal Mesh because at the moment we can only add to a skeletal mesh not make one."));
						FMessageDialog::Open(EAppMsgType::Ok, DialogText);
					}
				} else if (Type == "AnimSequence" || Type == "AnimMontage") {
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
					if (UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(Asset)) {
						EvaluateAnimSequence(DataObject.Get(), AnimSequenceBase);
					}
				} else if (Type == "ReverbEffect") {
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

					if (Properties->TryGetNumberField("AirAbsorptionGainHF", AirAbsorptionGainHF)) {
						ReverbEffect->AirAbsorptionGainHF = Properties->GetNumberField("AirAbsorptionGainHF");
					}
					if (Properties->TryGetNumberField("DecayHFRatio", DecayHFRatio)) {
						ReverbEffect->DecayHFRatio = Properties->GetNumberField("DecayHFRatio");
					}
					if (Properties->TryGetNumberField("DecayTime", DecayTime)) {
						ReverbEffect->DecayTime = Properties->GetNumberField("DecayTime");
					}
					if (Properties->TryGetNumberField("Density", Density)) {
						ReverbEffect->Density = Properties->GetNumberField("Density");
					}
					if (Properties->TryGetNumberField("Diffusion", Diffusion)) {
						ReverbEffect->Diffusion = Properties->GetNumberField("Diffusion");
					}
					if (Properties->TryGetNumberField("Gain", Gain)) {
						ReverbEffect->Gain = Properties->GetNumberField("Gain");
					}
					if (Properties->TryGetNumberField("GainHF", GainHF)) {
						ReverbEffect->GainHF = Properties->GetNumberField("GainHF");
					}
					if (Properties->TryGetNumberField("LateDelay", LateDelay)) {
						ReverbEffect->LateDelay = Properties->GetNumberField("LateDelay");
					}
					if (Properties->TryGetNumberField("LateGain", LateGain)) {
						ReverbEffect->LateGain = Properties->GetNumberField("LateGain");
					}
					if (Properties->TryGetNumberField("ReflectionsDelay", ReflectionsDelay)) {
						ReverbEffect->ReflectionsDelay = Properties->GetNumberField("ReflectionsDelay");
					}
					if (Properties->TryGetNumberField("ReflectionsGain", ReflectionsGain)) {
						ReverbEffect->ReflectionsGain = Properties->GetNumberField("ReflectionsGain");
					}
					if (Properties->TryGetNumberField("RoomRolloffFactor", RoomRolloffFactor)) {
						ReverbEffect->RoomRolloffFactor = Properties->GetNumberField("RoomRolloffFactor");
					}
					if (Properties->TryGetNumberField("RoomRolloffFactor", RoomRolloffFactor)) {
						ReverbEffect->RoomRolloffFactor = Properties->GetNumberField("RoomRolloffFactor");
					}
					if (Properties->TryGetBoolField("bChanged", bChanged)) {
						ReverbEffect->bChanged = Properties->GetBoolField("bChanged");
					}
					if (Properties->TryGetBoolField("bBypassEarlyReflections", bBypassEarlyReflections)) {
						ReverbEffect->bBypassEarlyReflections = Properties->GetBoolField("bBypassEarlyReflections");
					}
					if (Properties->TryGetBoolField("bBypassLateReflections", bBypassLateReflections)) {
						ReverbEffect->bBypassLateReflections = Properties->GetBoolField("bBypassLateReflections");
					}

					FAssetRegistryModule::AssetCreated(ReverbEffect);
					ReverbEffect->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					ReverbEffect->PostEditChange();
					ReverbEffect->AddToRoot();
				} else if (Type == "MorphTarget") {
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					UPackage* Package = CreateAssetPackage(Name, OutFileNames);
					USkeleton* Skeleton = Cast<USkeleton>(GetSelectedAsset());

					USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(Skeleton, *Name, RF_Public | RF_Standalone);

					FAssetRegistryModule::AssetCreated(SkeletalMesh);
					SkeletalMesh->MarkPackageDirty();
					Package->SetDirtyFlag(true);
					SkeletalMesh->PostEditChange();
					SkeletalMesh->AddToRoot();
				} else if (Type == "StaticMeshComponent") {
					TSharedPtr<FJsonObject> Properties = DataObject->GetObjectField("Properties");

					// Get World from Viewport
					UWorld* World = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport)->World();
					// Create Actor (StaticMeshActor)
					AStaticMeshActor* StaticMesh = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass());

					StaticMesh->SetMobility(EComponentMobility::Static);
					StaticMesh->SetActorLabel(DataObject->GetStringField("Outer"));

					// Set Transform
					const TSharedPtr<FJsonObject>* ActorLoc;

					if (Properties->TryGetObjectField("RelativeLocation", ActorLoc)) {
						StaticMesh->SetActorLocation(ObjectToVector(ActorLoc->Get()));
					}

					const TSharedPtr<FJsonObject>* ActorRot;

					if (Properties->TryGetObjectField("RelativeRotation", ActorRot)) {
						StaticMesh->SetActorRotation(ObjectToRotator(ActorRot->Get()));
					}

					const TSharedPtr<FJsonObject>* ActorScale;

					if (Properties->TryGetObjectField("RelativeScale3D", ActorScale)) {
						StaticMesh->SetActorScale3D(ObjectToVector(ActorScale->Get()));
					}
					// ^ 
					// | completed

					const TSharedPtr<FJsonObject>* StaticMeshAs;

					if (Properties->TryGetObjectField("StaticMesh", StaticMeshAs)) {
						FString MeshName;
						StaticMeshAs->Get()->GetStringField("ObjectName").Split(" ", nullptr, &MeshName);
						FString MeshPath;

						StaticMeshAs->Get()->GetStringField("ObjectPath").Replace(TEXT("FortniteGame/Content/"), TEXT("/Game/")).Split(
							".", &MeshPath, nullptr);

						UStaticMesh* StaticMeshAsset = LoadObject<UStaticMesh>(nullptr, *(MeshPath + "." + MeshName));

						UStaticMeshComponent* MeshComponent = StaticMesh->GetStaticMeshComponent();
						if (MeshComponent) {
							MeshComponent->SetStaticMesh(StaticMeshAsset);
							GLog->Log(MeshPath + "." + MeshName);
							GLog->Log(StaticMeshAsset->GetPathName());
						}
					}
				}
			}
		}
	}
}

void FJsonAsAssetModule::RegisterMenus() {
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

FVector FJsonAsAssetModule::ObjectToVector(FJsonObject* Object) {
	return FVector(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"));
}

FRotator FJsonAsAssetModule::ObjectToRotator(FJsonObject* Object) {
	return FRotator(Object->GetNumberField("Pitch"), Object->GetNumberField("Yaw"), Object->GetNumberField("Roll"));
}

FQuat FJsonAsAssetModule::ObjectToQuat(FJsonObject* Object) {
	return FQuat(Object->GetNumberField("X"), Object->GetNumberField("Y"), Object->GetNumberField("Z"), Object->GetNumberField("W"));
}

FLinearColor FJsonAsAssetModule::ObjectToLinearColor(FJsonObject* Object) {
	return FLinearColor(Object->GetNumberField("R"), Object->GetNumberField("G"), Object->GetNumberField("B"), Object->GetNumberField("A"));
}

bool FJsonAsAssetModule::EvaluateAnimSequence(FJsonObject* Object, UAnimSequenceBase* AnimSequenceBase) {
	// Properties of the object
	TSharedPtr<FJsonObject> Properties = Object->GetObjectField("Properties");

	TArray<TSharedPtr<FJsonValue>> FloatCurves;
	TArray<TSharedPtr<FJsonValue>> Notifies;
	TArray<TSharedPtr<FJsonValue>> AuthoredSyncMarkers;
	const TArray<TSharedPtr<FJsonValue>>* AuthoredSyncMarkers1;
	const TArray<TSharedPtr<FJsonValue>>* Notifiesa;
	const TSharedPtr<FJsonObject>* RawCurveData;

	if (Properties->TryGetObjectField("RawCurveData", RawCurveData)) {
		FloatCurves = Properties->GetObjectField("RawCurveData")->GetArrayField("FloatCurves");
	} else {
		if (Object->TryGetObjectField("CompressedCurveData", RawCurveData)) {
			FloatCurves = Object->GetObjectField("CompressedCurveData")->GetArrayField("FloatCurves");
		}
	}

	for (TSharedPtr<FJsonValue> FloatCurveObject : FloatCurves) {
		TArray<TSharedPtr<FJsonValue>> Keys = FloatCurveObject->AsObject()->GetObjectField("FloatCurve")->GetArrayField("Keys");

		GLog->Log("JsonAsAsset: Added animation curve: " + FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName"));

		USkeleton* Skeleton = AnimSequenceBase->GetSkeleton();
		FSmartName NewTrackName;

		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName,
		                                FName(*FloatCurveObject->AsObject()->GetObjectField("Name")->GetStringField("DisplayName")), NewTrackName);

		int CurveTypeFlags = FloatCurveObject->AsObject()->GetIntegerField("CurveTypeFlags");

		ensureAlways(Skeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, NewTrackName.UID, NewTrackName));
		// AnimSequenceBase->RawCurveData.AddCurveData(NewTrackName, CurveTypeFlags);

		for (int32 key_index = 0; key_index < Keys.Num(); key_index++) {
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

	if (Properties->TryGetArrayField("Notifies", Notifiesa)) {
		Notifies = Properties->GetArrayField("Notifies");

		for (TSharedPtr<FJsonValue> Notify : Notifies) {
		}
	}

	UAnimSequence* CastedAnimSequence = Cast<UAnimSequence>(AnimSequenceBase);

	if (Properties->TryGetArrayField("AuthoredSyncMarkers", AuthoredSyncMarkers1) && CastedAnimSequence) {
		AuthoredSyncMarkers = Properties->GetArrayField("AuthoredSyncMarkers");

		for (TSharedPtr<FJsonValue> SyncMarker : AuthoredSyncMarkers) {
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

UObject* FJsonAsAssetModule::GetSelectedAsset() {
	// The content browser has a module, which helps us extract the
	// selected assets, as it is completely connected.
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.Num() == 0) {
		GLog->Log("JsonAsAsset: [GetSelectedAsset] None selected, returning nullptr.");

		FText DialogText = FText::FromString(TEXT("A function to find a selected asset failed, please select a asset to go further."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		// None found, therefore we need to return nullptr.
		return nullptr;
	}

	// Return only the first selected asset
	return SelectedAssets[0].GetAsset();
}

UPackage* FJsonAsAssetModule::CreateAssetPackage(const FString& Name, const TArray<FString>& Files) const {
	UPackage* Ignore = nullptr;
	return CreateAssetPackage(Name, Files, Ignore);
}

UPackage* FJsonAsAssetModule::CreateAssetPackage(const FString& Name, const TArray<FString>& Files, UPackage*& OutOutermostPkg) const {
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
