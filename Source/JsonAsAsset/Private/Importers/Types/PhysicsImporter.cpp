// Copyright JAA Contributors 2024-2025

#include "Importers/Types/PhysicsImporter.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "Dom/JsonObject.h"
#if ENGINE_MINOR_VERSION >= 5
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif
#include "Utilities/AssetUtilities.h"

#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsAssetUtils.h"

bool UPhysicsImporter::ImportData() {
	try {
		USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(FAssetUtilities::GetSelectedAsset());

		UPhysicsAsset* PhysicsAsset = NewObject<UPhysicsAsset>(Package, *FileName, RF_Public | RF_Standalone | RF_Transactional);
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");

		PhysicsAsset->PreviewSkeletalMesh = SkeletalMesh;
		SkeletalMesh->SetPhysicsAsset(PhysicsAsset);
		SkeletalMesh->MarkPackageDirty();

		// Remove some properties if we do them manually
		TSharedPtr<FJsonObject> SerializerProperties = TSharedPtr<FJsonObject>(Properties); {
			FString BlacklistedProperies[] = {
				TEXT("BoundsBodies"),
				TEXT("ConstraintSetup"),
				TEXT("SkeletalBodySetups")
			};

			// Check, and remove
			for (FString& Property : BlacklistedProperies)
				if (SerializerProperties->HasField(Property))
					SerializerProperties->RemoveField(Property);

			// Deserialize object properties
			GetObjectSerializer()->DeserializeObjectProperties(SerializerProperties, PhysicsAsset);
		}

		// Add Skeletal Mesh Bodies
		for (TSharedPtr<FJsonValue> JsonValue : AllJsonObjects) {
			TSharedPtr<FJsonObject> JsonObjectP = JsonValue->AsObject();
			FString Name = JsonObjectP->GetStringField(TEXT("Name"));
			TSharedPtr<FJsonObject> SubObjectProperties = JsonObjectP->GetObjectField(TEXT("Properties"));

			// Find Skeletal Bodies
			if (JsonObjectP->GetStringField(TEXT("Type")) == "SkeletalBodySetup") {
				USkeletalBodySetup* SkeletalBody = NewObject<USkeletalBodySetup>(PhysicsAsset, NAME_None, RF_Transactional);

				GetObjectSerializer()->DeserializeObjectProperties(SubObjectProperties, SkeletalBody);
				SkeletalBody->BodySetupGuid = FGuid(JsonObjectP->GetStringField(TEXT("BodySetupGuid")));

				int32 BodySetupIndex = PhysicsAsset->SkeletalBodySetups.Add(SkeletalBody);

				PhysicsAsset->UpdateBodySetupIndexMap();
				PhysicsAsset->UpdateBoundsBodiesArray();
			}
		}

		// Add Physics Constraints
		for (TSharedPtr<FJsonValue> JsonValue : AllJsonObjects) {
			TSharedPtr<FJsonObject> _JsonObject = JsonValue->AsObject();
			FString Name = _JsonObject->GetStringField(TEXT("Name"));
			TSharedPtr<FJsonObject> SubObjectProperties = _JsonObject->GetObjectField(TEXT("Properties"));

			// Find Skeletal Bodies
			if (_JsonObject->GetStringField(TEXT("Type")) == "PhysicsConstraintTemplate") {
				UPhysicsConstraintTemplate* Constraint = NewObject<UPhysicsConstraintTemplate>(PhysicsAsset, NAME_None, RF_Transactional);
				GetObjectSerializer()->DeserializeObjectProperties(SubObjectProperties, Constraint);
				Constraint->SetDefaultProfile(Constraint->DefaultInstance); {
					PhysicsAsset->ConstraintSetup.Add(Constraint); // Add constraint
				}
			}
		}

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(PhysicsAsset)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}