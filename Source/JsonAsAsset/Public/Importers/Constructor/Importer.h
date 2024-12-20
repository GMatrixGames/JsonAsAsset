// Copyright JAA Contributors 2024-2025

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "../../Utilities/ObjectUtilities.h"
#include "../../Utilities/PropertyUtilities.h"
#include "Widgets/Notifications/SNotificationList.h"

// Global handler for converting JSON to assets
class IImporter {
public:
    IImporter()
        : PropertySerializer(nullptr), GObjectSerializer(nullptr),
          Package(nullptr), OutermostPkg(nullptr) {}

    IImporter(const FString& FileName, const FString& FilePath, 
              const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, 
              UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects = {})
        : FileName(FileName), FilePath(FilePath), JsonObject(JsonObject),
          Package(Package), OutermostPkg(OutermostPkg), AllJsonObjects(AllJsonObjects) 
    {
        PropertySerializer = NewObject<UPropertySerializer>();
        GObjectSerializer = NewObject<UObjectSerializer>();
        GObjectSerializer->SetPropertySerializer(PropertySerializer);
    }

    virtual ~IImporter() {}

    // Import the data of the supported type, return if successful or not
    virtual bool ImportData() { return false; }

protected:
    UPropertySerializer* PropertySerializer;
    UObjectSerializer* GObjectSerializer;

private:
    inline static TArray<FString> AcceptedTypes = {
        "CurveTable", "CurveFloat", "CurveVector", "CurveLinearColor", 
        "CurveLinearColorAtlas", "", "SkeletalMeshLODSettings", "Skeleton", 
        "", "AnimSequence", "AnimMontage", "", "Material", "MaterialFunction", 
        "MaterialInstanceConstant", "MaterialParameterCollection", 
        "NiagaraParameterCollection", "", "DataAsset", "LandscapeGrassType", 
        "DataTable", "", "PhysicsAsset", "PhysicalMaterial", "", "SoundCue", 
        "ReverbEffect", "SoundAttenuation", "SoundConcurrency", "SoundClass", 
        "SoundMix", "SoundModulationPatch", "", "SubsurfaceProfile", "", 
        "TextureRenderTarget2D"
    };

public:
    static TArray<FString> GetAcceptedTypes() { return AcceptedTypes; }

    template <class T = UObject>
    void LoadObject(const TSharedPtr<FJsonObject>* PackageIndex, TObjectPtr<T>& Object);

    template <class T = UObject>
    TArray<TObjectPtr<T>> LoadObject(const TArray<TSharedPtr<FJsonValue>>& PackageArray, TArray<TObjectPtr<T>> Array);

    static bool CanImport(const FString& ImporterType) { 
        return AcceptedTypes.Contains(ImporterType); 
    }

    static bool CanImportAny(TArray<FString>& Types) {
        for (FString& Type : Types) {
            if (CanImport(Type)) return true;
        }
        return false;
    }

    void ImportReference(const FString& File);
    bool ImportAssetReference(const FString& GamePath);
    bool ImportExports(TArray<TSharedPtr<FJsonValue>> Exports, FString File, bool bHideNotifications = false);

    TSharedPtr<FJsonObject> GetExport(FJsonObject* PackageIndex);

    // Notification Functions
    virtual void AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons = false, float WidthOverride = 500);
    virtual void AppendNotification(const FText& Text, const FText& SubText, float ExpireDuration, const FSlateBrush* SlateBrush, SNotificationItem::ECompletionState CompletionState, bool bUseSuccessFailIcons = false, float WidthOverride = 500);

    TSharedPtr<FJsonObject> RemovePropertiesShared(TSharedPtr<FJsonObject> Input, TArray<FString> RemovedProperties) const;

protected:
    bool HandleAssetCreation(UObject* Asset) const;
    void SavePackage();

    template <class T = UObject>
    TObjectPtr<T> DownloadWrapper(TObjectPtr<T> InObject, FString Type, FString Name, FString Path);

    static FName GetExportNameOfSubobject(const FString& PackageIndex);
    TArray<TSharedPtr<FJsonValue>> FilterExportsByOuter(const FString& Outer);
    TSharedPtr<FJsonValue> GetExportByObjectPath(const TSharedPtr<FJsonObject>& Object);

    FORCEINLINE UObjectSerializer* GetObjectSerializer() const { return GObjectSerializer; }

    FString FileName;
    FString FilePath;
    TSharedPtr<FJsonObject> JsonObject;
    UPackage* Package;
    UPackage* OutermostPkg;

    TArray<TSharedPtr<FJsonValue>> AllJsonObjects;
};