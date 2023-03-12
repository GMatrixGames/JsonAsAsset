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
#include "Importers/ReverbEffectImporter.h"
#include "Importers/SubsurfaceProfileImporter.h"
#include "Importers/AnimationBaseImporter.h"

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
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FJsonAsAssetStyle::Shutdown();
	FJsonAsAssetCommands::Unregister();
}

void FJsonAsAssetModule::PluginButtonClicked() {
	// Dialog for a JSON File
	TArray<FString> OutFileNames = OpenFileDialog(FString("Open a JSON file"), FString("JSON Files|*.json"));

	// None selected
	if (OutFileNames.Num() == 0) {
		FText DialogText = FText::FromString(TEXT(
			"A file is required to use this plugin, please select one next time."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		return;
	}

	// File name
	FString File = OutFileNames[0];

	/*----------------------  Parse JSON into UE JSON Reader---------------------- */
	FString ContentBefore;
	FFileHelper::LoadFileToString(ContentBefore, *File);

	FString Content = FString(TEXT("{\"data\": "));
	Content.Append(ContentBefore);
	Content.Append(FString("}"));

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
	/* ---------------------- ---------------------- ------------------------- */

	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		GLog->Log("JsonAsAsset: Deserialized file, reading the contents.");
		TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField("data");

		// Validation
		TArray<FString> Types;
		for (TSharedPtr<FJsonValue>& Obj : DataObjects) Types.Add(Obj->AsObject()->GetStringField("Type"));

		// If type is not supported, decline
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

				// TODO: Make this much cleaner
				if (Type == "CurveFloat") Importer = new UCurveFloatImporter(Name, DataObject, Package, OutermostPkg);
				else if (Type == "CurveLinearColor") Importer = new UCurveLinearColorImporter(Name, DataObject, Package, OutermostPkg);
				else if (Type == "AnimSequence") Importer = new UAnimationBaseImporter(Name, DataObject, Package, OutermostPkg, DataObjects);

				else if (Type == "SkeletalMeshLODSettings") Importer = new USkeletalMeshLODSettingsImporter(Name, DataObject, Package, OutermostPkg);
				else if (Type == "Skeleton") Importer = new USkeletonImporter(Name, DataObject, Package, OutermostPkg, DataObjects);

				else if (Type == "DataTable") Importer = new UDataTableImporter(Name, DataObject, Package, OutermostPkg);
				else if (Type == "SoundAttenuation") Importer = new USoundAttenuationImporter(Name, DataObject, Package, OutermostPkg);
				else if (Type == "ReverbEffect") Importer = new UReverbEffectImporter(Name, DataObject, Package, OutermostPkg);
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

TArray<FString> FJsonAsAssetModule::OpenFileDialog(FString Title, FString Type)
{
	TArray<FString> ReturnValue;

	// Window Handler for Windows
	void* ParentWindowHandle = nullptr;

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();

	// Define the window handle, if it's valid
	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		uint32 SelectionFlag = 0;
		// JSON Files|*.json
		DesktopPlatform->OpenFileDialog(ParentWindowHandle, Title, TEXT(""), FString(""), Type, SelectionFlag,
			ReturnValue);
	}

	return ReturnValue;
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

	OutputPath.Split("FortniteGame/Content", nullptr, &Path);
	Path.Split("/", &Path, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	const FString PathWithGame = "/Game" + Path + "/" + Name;
	UPackage* Package = CreatePackage(*PathWithGame);
	OutOutermostPkg = Package->GetOutermost();
	Package->FullyLoad();

	GLog->Log(Path);

	return Package;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)
