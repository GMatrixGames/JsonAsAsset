// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/ObjectUtilities.h"
#include "Utilities/PropertyUtilities.h"
// #include "Importers/Importer.h"
#include "UObject/Package.h"

DECLARE_LOG_CATEGORY_CLASS(LogObjectSerializer, All, All);
PRAGMA_DISABLE_OPTIMIZATION

TSet<FName> UObjectSerializer::UnhandledNativeClasses;

// Object Compare Settings ---------------
FObjectCompareSettings::FObjectCompareSettings() :
	bCheckObjectName(true),
	bCheckObjectOuter(true) {
}
FObjectCompareSettings::FObjectCompareSettings(bool bCheckObjectName, bool bCheckObjectOuter) {
	this->bCheckObjectName = bCheckObjectName;
	this->bCheckObjectOuter = bCheckObjectOuter;
}
// ----------------------------------------

FObjectCompareContext::FObjectCompareContext() { }

void FObjectCompareContext::SetObjectSettings(int32 ObjectIndex, const FObjectCompareSettings& Settings) {
	this->CompareSettings.Add(ObjectIndex, Settings);
}

bool FObjectCompareContext::HasObjectAlreadyBeenCompared(int32 ObjectIndex, UObject* Object) {
	const auto ObjectPair = TPair<int32, UObject*>(ObjectIndex, Object);
	if (ObjectsAlreadyCompared.Contains(ObjectPair)) {
		return true;
	}
	ObjectsAlreadyCompared.Add(ObjectPair);
	return false;
}

FObjectCompareSettings FObjectCompareContext::GetObjectSettings(int32 ObjectIndex) const {
	FObjectCompareSettings const* Settings = CompareSettings.Find(ObjectIndex);
	if (Settings == NULL) {
		return FObjectCompareSettings();
	}
	return *Settings;
}

// ----------------------------------------

UObjectSerializer::UObjectSerializer() {
	this->LastObjectIndex = 0;
}

UPackage* FindOrLoadPackage(const FString& PackageName) {
	UPackage* Package = FindPackage(NULL, *PackageName);
	if (!Package)
		Package = LoadPackage(NULL, *PackageName, LOAD_None);

	return Package;
}

void UObjectSerializer::SetPropertySerializer(UPropertySerializer* NewPropertySerializer) {
	check(NewPropertySerializer);

	this->PropertySerializer = NewPropertySerializer;
	NewPropertySerializer->ObjectSerializer = this;
}

void UObjectSerializer::InitializeForDeserialization(const TArray<TSharedPtr<FJsonValue>>& ObjectsArray) {
	this->LastObjectIndex = ObjectsArray.Num();

	for (int32 i = 0; i < LastObjectIndex; i++) {
		SerializedObjects.Add(i, ObjectsArray[i]->AsObject());
	}
}

void UObjectSerializer::InitializeForSerialization(UPackage* NewSourcePackage) {
	check(NewSourcePackage);
	this->SourcePackage = NewSourcePackage;
}

void UObjectSerializer::SetPackageForDeserialization(UPackage* SelfPackage) {
	check(SelfPackage);
	this->SourcePackage = SelfPackage;
}

void UObjectSerializer::SetObjectMark(UObject* Object, const FString& ObjectMark) {
	UObject* const* OldMarkObjectValue = ObjectMarks.FindKey(ObjectMark);

	// If we have an old value for this mark and it's the same object, exit early
	if (OldMarkObjectValue != NULL && *OldMarkObjectValue == Object) {
		return;
	}

	this->ObjectMarks.Add(Object, ObjectMark);

	// If we have an old mapping, remove it immediately and try to remap old objects to new ones
	if (OldMarkObjectValue != NULL) {
		this->ObjectMarks.Remove(*OldMarkObjectValue);

		TArray<int32> IndicesToOverwrite;
		for (const TPair<int32, UObject*>& Pair : this->LoadedObjects) {
			if (Pair.Value == *OldMarkObjectValue) {
				IndicesToOverwrite.Add(Pair.Key);
			}
		}
		for (const int32 ObjectIndex : IndicesToOverwrite) {
			this->LoadedObjects.Add(ObjectIndex, Object);
		}
	}
}

int32 UObjectSerializer::SerializeObject(UObject* Object) {
	if (Object == nullptr) 
		return INDEX_NONE;

	const int32* ObjectIndex = ObjectIndices.Find(Object);
	if (ObjectIndex != nullptr)
		return *ObjectIndex;

	const int32 NewObjectIndex = LastObjectIndex++;
	ObjectIndices.Add(Object, NewObjectIndex);
	UPackage* ObjectPackage = Object->GetOutermost();

	TSharedRef<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
	ResultJson->SetNumberField(TEXT("ObjectIndex"), NewObjectIndex);
	SerializedObjects.Add(NewObjectIndex, ResultJson);

	if (ObjectPackage != SourcePackage) {
		ResultJson->SetStringField(TEXT("Type"), TEXT("Import"));
		SerializeImportedObject(ResultJson, Object);
	} else {
		ResultJson->SetStringField(TEXT("Type"), TEXT("Export"));

		if (ObjectMarks.Contains(Object)) {
			// This object is serialized using object mark string
			ResultJson->SetStringField(TEXT("ObjectMark"), ObjectMarks.FindChecked(Object));
		} else {
			//Serialize object normally
			SerializeExportedObject(ResultJson, Object);
		}
	}

	return NewObjectIndex;
}

UObject* UObjectSerializer::DeserializeObject(int32 Index) {
	if (Index == INDEX_NONE)
		return nullptr;

	if (Index == 1)
		Index = 0;
	UObject** LoadedObject = LoadedObjects.Find(Index);
	if (LoadedObject != nullptr)
		return *LoadedObject;

	if (!SerializedObjects.Contains(Index)) {
		UE_LOG(LogObjectSerializer, Error, TEXT("DeserializeObject for package %s called with invalid Index: %d"), *SourcePackage->GetName(), Index);
		return nullptr;
	}

	const TSharedPtr<FJsonObject>& ObjectJson = SerializedObjects.FindChecked(Index);
	FString ObjectType = Index == 0 ? "Import" : "Export";

	if (ObjectType == TEXT("Import")) {
		// Object is imported from another package, and not located in our own
		UObject* NewLoadedObject = DeserializeImportedObject(ObjectJson);
		LoadedObjects.Add(Index, NewLoadedObject);

		return NewLoadedObject;
	}

	if (ObjectType == TEXT("Export")) {
		// Object is defined inside our own package
		UObject* ConstructedObject;

		if (ObjectJson->HasField(TEXT("ObjectMark"))) {
			// Object is serialized through object mark
			const FString ObjectMark = ObjectJson->GetStringField(TEXT("ObjectMark"));
			UObject* const* FoundObject = ObjectMarks.FindKey(ObjectMark);
			checkf(FoundObject, TEXT("Cannot resolve object serialized using mark: %s"), *ObjectMark);
			ConstructedObject = *FoundObject;
		} else {
			//Perform normal deserialization
			ConstructedObject = DeserializeExportedObject(Index, ObjectJson);
		}

		LoadedObjects.Add(Index, ConstructedObject);
		return ConstructedObject;
	}

	UE_LOG(LogObjectSerializer, Fatal, TEXT("Unhandled object type: %s for package %s"), *ObjectType, *SourcePackage->GetPathName());
	return nullptr;
}

TSharedRef<FJsonObject> UObjectSerializer::SerializeObjectProperties(UObject* Object) {
	TSharedRef<FJsonObject> Properties = MakeShareable(new FJsonObject());
	SerializeObjectPropertiesIntoObject(Object, Properties);
	return Properties;
}

void UObjectSerializer::SerializeObjectPropertiesIntoObject(UObject* Object, TSharedPtr<FJsonObject> Properties) {
	UClass* ObjectClass = Object->GetClass();
	TArray<int32> ReferencedSubobjects;

	// Serialize actual object property values
	for (UProperty* Property = ObjectClass->PropertyLink; Property; Property = Property->PropertyLinkNext) {
		if (PropertySerializer->ShouldSerializeProperty(Property)) {
			const void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Object);
			TSharedRef<FJsonValue> PropertyValueJson = PropertySerializer->SerializePropertyValue(Property, PropertyValue, &ReferencedSubobjects);

			Properties->SetField(Property->GetName(), PropertyValueJson);
		}
	}

	// Remove NULL from referenced subobjects because writing it down is useless
	ReferencedSubobjects.RemoveAllSwap([](const int32 ObjectIndex) { return ObjectIndex == -1; });

	// Also write $ReferencedSubobjects field used for deserialization dependency gathering
	TArray<TSharedPtr<FJsonValue>> ReferencedSubobjectsArray;
	for (const int32 ObjectIndex : ReferencedSubobjects) {
		ReferencedSubobjectsArray.Add(MakeShareable(new FJsonValueNumber(ObjectIndex)));
	}

	Properties->SetArrayField(TEXT("$ReferencedObjects"), ReferencedSubobjectsArray);
}

bool UObjectSerializer::CompareObjectsWithContext(const int32 ObjectIndex, UObject* Object, TSharedPtr<FObjectCompareContext> CompareContext) {
	// If either of the operands are null, they are only equal if both are NULL
	if (ObjectIndex == INDEX_NONE || Object == NULL) {
		if (ObjectIndex == INDEX_NONE && Object == NULL)
			return true;

		if (ObjectIndex == INDEX_NONE)
			return false;

		// If the object is not found, deserializing it would still be NULL
		const TSharedPtr<FJsonObject>& ObjectJson = SerializedObjects.FindChecked(ObjectIndex);
		const FString ObjectType = ObjectJson->GetStringField(TEXT("Type"));

		return ObjectType == TEXT("Import") && DeserializeObject(ObjectIndex) == NULL;
	}

	// Return true if we have already compared this object, otherwise we will run into the recursion
	// Actual comparison already yielded the result, so the overall graph comparison result will stay the same
	if (CompareContext->HasObjectAlreadyBeenCompared(ObjectIndex, Object))
		return true;

	const TSharedPtr<FJsonObject>& ObjectJson = SerializedObjects.FindChecked(ObjectIndex);
	const FString ObjectType = ObjectJson->GetStringField(TEXT("Type"));
	const FObjectCompareSettings CompareSettings = CompareContext->GetObjectSettings(ObjectIndex);

	// Object is imported from another package, and not located in our own
	if (ObjectType == TEXT("Import")) {
		// Return early if object name doesn't match
		const FString ObjectName = ObjectJson->GetStringField(TEXT("ObjectName"));
		if (Object->GetName() != ObjectName)
			return false;

		// If we have outer, compare them too to make sure they match
		if (ObjectJson->HasField(TEXT("Outer"))) {
			const int32 OuterObjectIndex = ObjectJson->GetIntegerField(TEXT("Outer"));
			return CompareObjectsWithContext(OuterObjectIndex, Object->GetOuter(), CompareContext);
		}

		// We end up here if we have no outer but have matching name, in which case we represent top-level object
		return true;
	}

	// Otherwise we are dealing with the exported object
	// Check if object is serialized through mark first
	if (ObjectJson->HasField(TEXT("ObjectMark"))) {
		const FString ObjectMark = ObjectJson->GetStringField(TEXT("ObjectMark"));
		UObject* const* FoundObject = ObjectMarks.FindKey(ObjectMark);

		checkf(FoundObject, TEXT("Cannot resolve object serialized using mark: %s"), *ObjectMark);
		UObject* RegisteredObject = *FoundObject;

		// Marked objects only match if they point to the same UObject
		return RegisteredObject == Object;
	}

	// Make sure object name matches first
	const FString ObjectName = ObjectJson->GetStringField(TEXT("ObjectName"));
	if (CompareSettings.bCheckObjectName && Object->GetName() != ObjectName)
		return false;

	// Make sure object class matches provided one
	const int32 ObjectClassIndex = ObjectJson->GetIntegerField(TEXT("ObjectClass"));
	if (!CompareObjectsWithContext(ObjectClassIndex, Object->GetClass(), CompareContext))
		return false;

	// If object is missing outer, we are dealing with the package itself. Then SourcePackage must match Object,
	// and we do not have any properties recorded for UPackage, so we return early
	if (!ObjectJson->HasField(TEXT("Outer")))
		return SourcePackage == Object;

	// Otherwise make sure outer object matches
	const int32 OuterObjectIndex = ObjectJson->GetIntegerField(TEXT("Outer"));
	if (CompareSettings.bCheckObjectOuter && !CompareObjectsWithContext(OuterObjectIndex, Object->GetOuter(), CompareContext))
		return false;

	// Deserialize object properties now
	if (ObjectJson->HasField(TEXT("Properties"))) {
		const TSharedPtr<FJsonObject>& Properties = ObjectJson->GetObjectField(TEXT("Properties"));

		return AreObjectPropertiesUpToDate(Properties.ToSharedRef(), Object, CompareContext);
	}

	// No properties detected, we are matching just fine in that case
	return true;
}

bool UObjectSerializer::AreObjectPropertiesUpToDate(const TSharedPtr<FJsonObject>& Properties, UObject* Object, const TSharedPtr<FObjectCompareContext> Context) {
	UClass* ObjectClass = Object->GetClass();

	// Iterate all properties and return false if our values do not match existing ones
	// 
	// This will also try to deserialize objects in "read only" mode, incrementing 
	// ObjectsNotUpToDate when existing object fields mismatch
	for (UProperty* Property = ObjectClass->PropertyLink; Property; Property = Property->PropertyLinkNext) {
		const FString PropertyName = Property->GetName();

		if (PropertySerializer->ShouldSerializeProperty(Property) && Properties->HasField(PropertyName)) {
			const void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Object);
			const TSharedPtr<FJsonValue> ValueObject = Properties->Values.FindChecked(PropertyName);

			if (!PropertySerializer->ComparePropertyValues(Property, ValueObject.ToSharedRef(), PropertyValue, Context)) {
				return false;
			}
		}
	}

	return true;
}

void UObjectSerializer::FlushPropertiesIntoObject(const int32 ObjectIndex, UObject* Object, const bool bVerifyNameAndRename, const bool bVerifyOuterAndMove) {
	check(ObjectIndex != INDEX_NONE);
	check(Object);

	const TSharedPtr<FJsonObject> ObjectData = this->SerializedObjects.FindChecked(ObjectIndex);
	if (ObjectData->Values.Num() == 0) return; // If the object entry is empty, ignore

	checkf(!this->LoadedObjects.Contains(ObjectIndex), TEXT("Cannot flush properties into already deserialized object"));
	this->LoadedObjects.Add(ObjectIndex, Object);

	const FString ObjectType = ObjectData->GetStringField(TEXT("Type"));
	checkf(ObjectType == TEXT("Export"), TEXT("Can only call FlushPropertiesIntoObject for exported objects"));

	const int32 ObjectClassIndex = ObjectData->GetIntegerField(TEXT("ObjectClass"));
	UObject* ObjectClassRaw = DeserializeObject(ObjectClassIndex);

	UClass* ObjectClass = CastChecked<UClass>(ObjectClassRaw);
	checkf(Object->GetClass()->IsChildOf(ObjectClass), TEXT("Can only call FlushPropertiesIntoObject for objects matching serialized object class"));

	if (bVerifyNameAndRename) {
		const FString ObjectName = ObjectData->GetStringField(TEXT("ObjectName"));
		if (Object->GetName() != ObjectName)
			Object->Rename(*ObjectName, NULL, REN_ForceNoResetLoaders);
	}
	if (bVerifyOuterAndMove) {
		const int32 OuterObjectIndex = ObjectData->GetIntegerField(TEXT("Outer"));
		UObject* OuterObject = DeserializeObject(OuterObjectIndex);

		if (Object->GetOuter() != OuterObject)
			Object->Rename(NULL, OuterObject, REN_ForceNoResetLoaders);
	}

	const TSharedPtr<FJsonObject> Properties = ObjectData->GetObjectField(TEXT("Properties"));
	DeserializeObjectProperties(Properties.ToSharedRef(), Object);
}

void UObjectSerializer::DeserializeObjectProperties(const TSharedPtr<FJsonObject>& Properties, UObject* Object) {
	UClass* ObjectClass = Object->GetClass();

	for (UProperty* Property = ObjectClass->PropertyLink; Property; Property = Property->PropertyLinkNext) {
		const FString PropertyName = Property->GetName();

		if (PropertySerializer->ShouldSerializeProperty(Property) && Properties->HasField(PropertyName)) {
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Object);
			const TSharedPtr<FJsonValue>& ValueObject = Properties->Values.FindChecked(PropertyName);

			PropertySerializer->DeserializePropertyValue(Property, ValueObject.ToSharedRef(), PropertyValue);
		}
	}
}

TArray<TSharedPtr<FJsonValue>> UObjectSerializer::FinalizeSerialization() {
	TArray<TSharedPtr<FJsonValue>> ObjectsArray;

	for (int32 i = 0; i < LastObjectIndex; i++) {
		if (!SerializedObjects.Contains(i)) {
			checkf(false, TEXT("Object not in serialized objects: %s"), *(*ObjectIndices.FindKey(i))->GetPathName());
		}
		
		ObjectsArray.Add(MakeShareable(new FJsonValueObject(SerializedObjects.FindChecked(i))));
	}
	return ObjectsArray;
}

void UObjectSerializer::CollectReferencedPackages(const TArray<TSharedPtr<FJsonValue>>& ReferencedSubobjects, TArray<FString>& OutReferencedPackageNames) {
	TArray<int32> AlreadySerializedObjects;
	CollectReferencedPackages(ReferencedSubobjects, OutReferencedPackageNames, AlreadySerializedObjects);
}

void UObjectSerializer::CollectReferencedPackages(const TArray<TSharedPtr<FJsonValue>>& ReferencedSubobjects, TArray<FString>& OutReferencedPackageNames, TArray<int32>& ObjectsAlreadySerialized) {
	for (const TSharedPtr<FJsonValue>& JsonValue : ReferencedSubobjects) {
		CollectObjectPackages(JsonValue->AsNumber(), OutReferencedPackageNames, ObjectsAlreadySerialized);
	}
}

void UObjectSerializer::CollectObjectPackages(const int32 ObjectIndex, TArray<FString>& OutReferencedPackageNames, TArray<int32>& ObjectsAlreadySerialized) {
	if (ObjectIndex == INDEX_NONE)
		return;
	if (ObjectsAlreadySerialized.Contains(ObjectIndex))
		return;

	ObjectsAlreadySerialized.Add(ObjectIndex);
	const TSharedPtr<FJsonObject> Object = SerializedObjects.FindChecked(ObjectIndex);
	const FString ObjectType = Object->GetStringField(TEXT("Type"));

	if (ObjectType == TEXT("Import")) {
		const FString ClassPackage = Object->GetStringField(TEXT("ClassPackage"));
		if (!ClassPackage.StartsWith(TEXT("/Script/")))
			OutReferencedPackageNames.AddUnique(ClassPackage);

		if (Object->HasField(TEXT("Outer"))) {
			const int32 OuterObjectIndex = Object->GetIntegerField(TEXT("Outer"));
			CollectObjectPackages(OuterObjectIndex, OutReferencedPackageNames, ObjectsAlreadySerialized);
		} else {
			const FString PackageName = Object->GetStringField(TEXT("ObjectName"));
			OutReferencedPackageNames.AddUnique(PackageName);
		}
	} else if (ObjectType == TEXT("Export")) {
		if (Object->HasField(TEXT("ObjectMark")))
			return;

		const int32 ObjectClassIndex = Object->GetIntegerField(TEXT("ObjectClass"));
		CollectObjectPackages(ObjectClassIndex, OutReferencedPackageNames, ObjectsAlreadySerialized);

		if (Object->HasField(TEXT("Outer"))) {
			const int32 OuterObjectIndex = Object->GetIntegerField(TEXT("Outer"));
			CollectObjectPackages(OuterObjectIndex, OutReferencedPackageNames, ObjectsAlreadySerialized);
		}

		if (Object->HasField(TEXT("Properties"))) {
			const TSharedPtr<FJsonObject> Properties = Object->GetObjectField(TEXT("Properties"));
			const TArray<TSharedPtr<FJsonValue>> ReferencedSubobjects = Properties->GetArrayField(TEXT("$ReferencedObjects"));

			CollectReferencedPackages(ReferencedSubobjects, OutReferencedPackageNames, ObjectsAlreadySerialized);
		}
	}
}

FString UObjectSerializer::GetObjectFullPath(int32 ObjectIndex) {
	const TSharedPtr<FJsonObject> Object = SerializedObjects.FindChecked(ObjectIndex);
	const FString ObjectType = Object->GetStringField(TEXT("Type"));

	if (ObjectType == TEXT("Import")) {
		FString ResultPath;

		if (Object->HasField(TEXT("Outer"))) {
			const int32 OuterObjectIndex = Object->GetIntegerField(TEXT("Outer"));
			ResultPath.Append(GetObjectFullPath(OuterObjectIndex));

			ResultPath.Append(TEXT("."));
		}

		const FString ObjectName = Object->GetStringField(TEXT("ObjectName"));
		ResultPath.Append(ObjectName);

		return ResultPath;
	}

	if (ObjectType == TEXT("Export")) {
		if (Object->HasField(TEXT("ObjectMark"))) {
			const FString ObjectMark = Object->GetStringField(TEXT("ObjectMark"));
			return FString::Printf(TEXT("ObjectMark('%s')"), *ObjectMark);
		}

		if (!Object->HasField(TEXT("Outer"))) {
			return TEXT("SelfPackage()");
		}

		FString ResultPath;
		const int32 OuterObjectIndex = Object->GetIntegerField(TEXT("Outer"));
		ResultPath.Append(GetObjectFullPath(OuterObjectIndex));
		ResultPath.Append(TEXT("."));

		const FString ObjectName = Object->GetStringField(TEXT("ObjectName"));
		ResultPath.Append(ObjectName);

		return ResultPath;
	}

	checkf(0, TEXT("Unknown object type: %s"), *ObjectType);
	return TEXT("");
}
UObject* UObjectSerializer::DeserializeExportedObject(int32 ObjectIndex, TSharedPtr<FJsonObject> ObjectJson) {
	// Object is defined inside our own package, so we should have
	// NOTE: Probably shouldn't be a export
	const FString ObjectClassIndex = ObjectJson->GetStringField(TEXT("Type"));
	const FString PackageName = "/Script/Engine";

	UPackage* ClassPackage = FindOrLoadPackage(PackageName);
	UClass* ObjectClass = FindObjectFast<UClass>(ClassPackage, *ObjectClassIndex);

	if (ObjectClass == nullptr) {
		UE_LOG(LogObjectSerializer, Error, TEXT("DeserializeObject for package %s failed: Cannot resolve object class %s"), *SourcePackage->GetName(), *ObjectClassIndex);
		return nullptr;
	}

	const FString ObjectName = ObjectJson->GetStringField(TEXT("Name"));
	const FString PackageSpot = ObjectJson->GetStringField(TEXT("Package"));

	// Outer will be missing for root UPackage export, e.g SourcePackage
	if (!ObjectJson->HasField(TEXT("Outer"))) {
		return SourcePackage;
	}
	
	if (ObjectJson->HasField(TEXT("bIsPackaged"))) {
		UPackage* ResultPackage = FindOrLoadPackage(PackageSpot);

		return StaticFindObjectFast(ObjectClass, ResultPackage, *ObjectName);
	}

	const int32 OuterObjectIndex = ObjectJson->GetIntegerField(TEXT("Outer"));
	UObject* OuterObject = DeserializeObject(OuterObjectIndex);

	if (OuterObject == nullptr) {
		UE_LOG(LogObjectSerializer, Error, TEXT("DeserializeObject for package %s failed: Cannot resolve outer object %d"), *SourcePackage->GetName(), OuterObjectIndex);
		
		return nullptr;
	}

	UObject* ConstructedObject;

	// Try to resolve existing object inside of the outer first
	if (UObject* ExistingObject = StaticFindObjectFast(ObjectClass, OuterObject, *ObjectName)) {
		ConstructedObject = ExistingObject;
	} else {
		// Construct new object if we cannot otherwise
		const EObjectFlags ObjectLoadFlags = (EObjectFlags)ObjectJson->GetIntegerField(TEXT("ObjectFlags"));
		UObject* Template = GetArchetypeFromRequiredInfo(ObjectClass, OuterObject, *ObjectName, ObjectLoadFlags);
		ConstructedObject = StaticConstructObject_Internal(ObjectClass, OuterObject, *ObjectName, ObjectLoadFlags, EInternalObjectFlags::None, Template);
	}

	// Record constructed object so when properties reference it through outer chain we do not run into stack overflow
	LoadedObjects.Add(ObjectIndex, ConstructedObject);

	// Deserialize object properties now
	if (ObjectJson->HasField(TEXT("Properties"))) {
		const TSharedPtr<FJsonObject>& Properties = ObjectJson->GetObjectField(TEXT("Properties"));
		if (Properties.IsValid()) {
			DeserializeObjectProperties(Properties.ToSharedRef(), ConstructedObject);
		}
	}

	return ConstructedObject;
}

UObject* UObjectSerializer::DeserializeImportedObject(TSharedPtr<FJsonObject> ObjectJson) {
	const FString ObjectClassIndex = ObjectJson->GetStringField(TEXT("Type"));
	const FString PackageName = "/Script/Engine";
	UPackage* ClassPackage = FindOrLoadPackage(PackageName);
	
	const FString ClassName = ObjectJson->GetStringField(TEXT("ClassName"));
	const FString ObjectName = ObjectJson->GetStringField(TEXT("Name"));
	
	if (ClassPackage == NULL) {
		UE_LOG(LogObjectSerializer, Error, TEXT("Failed to resolve class package %s"), *PackageName);
		return NULL;
	}
	
	UClass* ObjectClass = FindObjectFast<UClass>(ClassPackage, *ObjectClassIndex);
	if (ObjectClass == NULL) {
		UE_LOG(LogObjectSerializer, Error, TEXT("Failed to resolve class %s inside of the package %s (requested by %s)"), *ClassName, *PackageName, *SourcePackage->GetName());
		return NULL;
	}

	// Outer is absent for root UPackage imports - Use ObjectName with LoadPackage directly
	if (ObjectJson->HasField(TEXT("bIsPackaged"))) {
		UPackage* ResultPackage = FindOrLoadPackage(ObjectName);
		if (ResultPackage == NULL)
			return NULL;

		return ResultPackage;
	}

	// Otherwise, it is a normal object inside some outer
	// have to actually find the outer here.
	int32 OuterObjectIndex = ObjectJson->GetIntegerField(TEXT("Outer"));
	UObject* OuterObject = (OuterObjectIndex == ObjectJson->GetIntegerField(TEXT("Outer"))) ? FindOrLoadPackage("/Script/Engine") : DeserializeObject(OuterObjectIndex);

	if (OuterObject == NULL) {
		UE_LOG(LogObjectSerializer, Error, TEXT("Cannot deserialize object %s because it's outer object %d failed deserialization (requested by %s)"), *ObjectName, OuterObjectIndex, *SourcePackage->GetName());
		return NULL;
	}

	// Use FindObjectFast now to resolve our object inside Outer
	UObject* ResultObject = StaticFindObjectFast(ObjectClass, OuterObject, *ObjectName);
	if (ResultObject == NULL) {
		UE_LOG(LogObjectSerializer, Error, TEXT("Cannot resolve object %s inside of the outer %s (requested by %s)"), *ObjectName, *OuterObject->GetPathName(), *SourcePackage->GetPathName());
		return NULL;
	}

	return ResultObject;
}

void UObjectSerializer::SerializeImportedObject(TSharedPtr<FJsonObject> ResultJson, UObject* Object) {
	// Object is imported from different package
	UClass* ObjectClass = Object->GetClass();
	ResultJson->SetStringField(TEXT("ClassPackage"), ObjectClass->GetOutermost()->GetName());
	ResultJson->SetStringField(TEXT("ClassName"), ObjectClass->GetName());
	UObject* OuterObject = Object->GetOuter();

	// Outer object can be null if import is the top UPackage object
	if (OuterObject != nullptr) {
		const int32 OuterObjectIndex = SerializeObject(OuterObject);
		ResultJson->SetNumberField(TEXT("Outer"), OuterObjectIndex);
	}

	ResultJson->SetStringField(TEXT("ObjectName"), Object->GetName());
}

void UObjectSerializer::SerializeExportedObject(TSharedPtr<FJsonObject> ResultJson, UObject* Object) {
	// Object is located inside our own package, so we need to serialize it properly
	UClass* ObjectClass = Object->GetClass();
	ResultJson->SetNumberField(TEXT("ObjectClass"), SerializeObject(ObjectClass));
	UObject* OuterObject = Object->GetOuter();

	// Object being serialized is this package itself
	// Make sure object is package and write only object class, that is enough
	if (OuterObject == NULL) {
		check(ObjectClass == UPackage::StaticClass());
		return;
	}

	const int32 OuterObjectIndex = SerializeObject(OuterObject);
	ResultJson->SetNumberField(TEXT("Outer"), OuterObjectIndex);
	ResultJson->SetStringField(TEXT("ObjectName"), Object->GetName());

	// Serialize object flags that match RF_Load specification
	ResultJson->SetNumberField(TEXT("ObjectFlags"), (int32)(Object->GetFlags() & RF_Load));

	// If we have native serializer set, let it decide whenever we want normal property serialization or not
	const bool bShouldSerializeProperties = true;

	// Serialize UProperties for this object if requested
	if (bShouldSerializeProperties) {
		const TSharedRef<FJsonObject> Properties = SerializeObjectProperties(Object);
		ResultJson->SetObjectField(TEXT("Properties"), Properties);
	}
}

PRAGMA_ENABLE_OPTIMIZATION