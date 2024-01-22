// Copyright JAA Contributors 2023-2024

#include "Utilities/EditorGraph/MaterialGraph_Interface.h"

#include "Dom/JsonObject.h"
#include "Utilities/MathUtilities.h"

// Expressions
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionFeatureLevelSwitch.h"
#include "Materials/MaterialExpressionShadingPathSwitch.h"
#include "Materials/MaterialExpressionQualitySwitch.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionGetMaterialAttributes.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionReroute.h"

#include <MaterialEditingLibrary.h>
#include <Editor/UnrealEd/Public/FileHelpers.h>

#include "Materials/MaterialExpressionTextureBase.h"
#include <Runtime/SlateCore/Public/Styling/SlateIconFinder.h>

static TWeakPtr<SNotificationItem> MaterialGraphNotification;

TSharedPtr<FJsonObject> UMaterialGraph_Interface::FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FImportData>& OutExports, TArray<FName>& ExpressionNames, bool bFilterByOuter) {
	TSharedPtr<FJsonObject> EditorOnlyData;

	for (const TSharedPtr<FJsonValue> Value : bFilterByOuter ? FilterExportsByOuter(Outer) : AllJsonObjects) {
		TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

		FString ExType = Object->GetStringField("Type");
		FString Name = Object->GetStringField("Name");

		if (ExType == Type + "EditorOnlyData") {
			EditorOnlyData = Object;
			continue;
		}

		// For older versions, the "editor" data is in the main UMaterial/UMaterialFunction export
		if (ExType == Type) {
			EditorOnlyData = Object;
			continue;
		}

		ExpressionNames.Add(FName(Name));
		OutExports.Add(FName(Name), FImportData(ExType, Outer, Object));
	}

	return EditorOnlyData;
}

TMap<FName, UMaterialExpression*> UMaterialGraph_Interface::ConstructExpressions(UObject* Parent, const FString& Outer, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports) {
	TMap<FName, UMaterialExpression*> CreatedExpressionMap;

	for (FName Name : ExpressionNames) {
		FName Type;
		FJsonObject* SharedRef = NULL;
		bool bFound = false;

		for (TTuple<FName, FImportData>& Key : Exports) {
			TSharedPtr<FJsonObject>* SharedO = new TSharedPtr<FJsonObject>(Key.Value.Json);

			if (Key.Key == Name && Key.Value.Outer == FName(*Outer)) {
				Type = Key.Value.Type;
				SharedRef = SharedO->Get();

				bFound = true;
				break;
			}
		}

		if (!bFound) continue;
		UMaterialExpression* Ex = CreateEmptyExpression(Parent, Name, Type, SharedRef);
		if (Ex == nullptr)
			continue;

		CreatedExpressionMap.Add(Name, Ex);
	}

	return CreatedExpressionMap;
}

FExpressionInput UMaterialGraph_Interface::CreateExpressionInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName) {
	// Find Expression Input by PropertyName
	if (const TSharedPtr<FJsonObject>* Ptr; JsonProperties->TryGetObjectField(PropertyName, Ptr)) {
		FJsonObject* AsObject = Ptr->Get();
		FName ExpressionName = GetExpressionName(AsObject);

		// If Expression Found
		if (CreatedExpressionMap.Contains(ExpressionName)) {
			FExpressionInput Input = PopulateExpressionInput(AsObject, *CreatedExpressionMap.Find(ExpressionName));

			return Input;
		}
	}

	return FExpressionInput();
}

FMaterialAttributesInput UMaterialGraph_Interface::CreateMaterialAttributesInput(TSharedPtr<FJsonObject> JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, FString PropertyName) {
	FExpressionInput Input = CreateExpressionInput(JsonProperties, CreatedExpressionMap, PropertyName);

	return *reinterpret_cast<FMaterialAttributesInput*>(&Input);
}

void UMaterialGraph_Interface::PropagateExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, bool bCheckOuter, bool bSubgraph) {
	for (FName Name : ExpressionNames) {
		FImportData* Type = Exports.Find(Name);
		TSharedPtr<FJsonObject> Properties = Type->Json->GetObjectField("Properties");

		// Find the expression from FName
		if (!CreatedExpressionMap.Contains(Name)) continue;
		UMaterialExpression* Expression = *CreatedExpressionMap.Find(Name);

		//	Used for Subgraphs:
		//  | Checks if the outer is the same as the parent
		//  | to determine if it's in a subgraph or not.
		if (bCheckOuter) {
			if (FString Outer;
				Type->Json->TryGetStringField("Outer", Outer) &&
				Outer != Parent->GetName()) // Not the same as parent

				continue;
		}

		// ------------ Manually check for Material Function Calls ------------ 
		if (Type->Type == "MaterialExpressionMaterialFunctionCall") {
			UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);

			if (const TSharedPtr<FJsonObject>* MaterialFunctionPtr; Properties->TryGetObjectField("MaterialFunction", MaterialFunctionPtr)) {
				LoadObject(MaterialFunctionPtr, MaterialFunctionCall->MaterialFunction);

				// Notify material function is missing
				if (MaterialFunctionCall->MaterialFunction == nullptr) {
					FString ObjectPath;
					MaterialFunctionPtr->Get()->GetStringField("ObjectPath").Split(".", &ObjectPath, nullptr);
					if (!HandleReference(ObjectPath)) {} // AppendNotification(FText::FromString("Material Function Missing: " + ObjectPath), FText::FromString("Material Graph"), 2.0f, SNotificationItem::CS_Fail, true);
					else LoadObject(MaterialFunctionPtr, MaterialFunctionCall->MaterialFunction);
				}
			}
		}

		// Sets 99% of properties for nodes
		GetObjectSerializer()->DeserializeObjectProperties(Properties, Expression);

		// Material Nodes with edited properties (ex: 9 objects with the same name ---> array of objects)
		if (Type->Type == "MaterialExpressionQualitySwitch") {
			UMaterialExpressionQualitySwitch* QualitySwitch = Cast<UMaterialExpressionQualitySwitch>(Expression);

			if (const TArray<TSharedPtr<FJsonValue>>* InputsPtr; Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						QualitySwitch->Inputs[i] = Input;
					}
					i++;
				}
			}
		} else if (Type->Type == "MaterialExpressionShadingPathSwitch") {
			UMaterialExpressionShadingPathSwitch* ShadingPathSwitch = Cast<UMaterialExpressionShadingPathSwitch>(Expression);

			if (const TArray<TSharedPtr<FJsonValue>>* InputsPtr; Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						ShadingPathSwitch->Inputs[i] = Input;
					}
					i++;
				}
			}
		} else if (Type->Type == "MaterialExpressionFeatureLevelSwitch") {
			UMaterialExpressionFeatureLevelSwitch* FeatureLevelSwitch = Cast<UMaterialExpressionFeatureLevelSwitch>(Expression);

			if (const TArray<TSharedPtr<FJsonValue>>* InputsPtr; Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						FeatureLevelSwitch->Inputs[i] = Input;
					}
					i++;
				}
			}
		}

		MaterialGraphNode_ExpressionWrapper(Parent, Expression, Properties);

		if (!bSubgraph) {
			if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent))
				FuncCasted->GetExpressionCollection().AddExpression(Expression);

			if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) {
				MatCasted->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
				Expression->UpdateMaterialExpressionGuid(true, false);
				MatCasted->AddExpressionParameter(Expression, MatCasted->EditorParameters);
			}
		}
	}
}

void UMaterialGraph_Interface::MaterialGraphNode_ConstructComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FImportData>& Exports) {
	if (const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments; Json->TryGetArrayField("EditorComments", StringExpressionComments))
		// Iterate through comments
		for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
			if (ExpressionComment->IsNull()) continue; // just in-case

			FName ExportName = GetExportNameOfSubobject(ExpressionComment.Get()->AsObject()->GetStringField("ObjectName"));

			// Get properties of comment, and create it relative to parent
			const TSharedPtr<FJsonObject> Properties = Exports.Find(ExportName)->Json->GetObjectField("Properties");
			UMaterialExpressionComment* Comment = NewObject<UMaterialExpressionComment>(Parent, UMaterialExpressionComment::StaticClass(), ExportName, RF_Transactional);

			// Deserialize and send it off to the material
			MaterialGraphNode_ExpressionWrapper(Parent, Comment, Properties);
			GetObjectSerializer()->DeserializeObjectProperties(Properties, Comment);

			if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) FuncCasted->GetExpressionCollection().AddComment(Comment);
			else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) MatCasted->GetExpressionCollection().AddComment(Comment);
		}

	for (TTuple<FString, FJsonObject*>& Key : MissingNodeClasses) {
		TSharedPtr<FJsonObject>* SharedObject = new TSharedPtr<FJsonObject>(Key.Value);

		const TSharedPtr<FJsonObject> Properties = SharedObject->Get()->GetObjectField("Properties");
		UMaterialExpressionComment* Comment = NewObject<UMaterialExpressionComment>(Parent, UMaterialExpressionComment::StaticClass(), *("UMaterialExpressionComment_" + Key.Key), RF_Transactional);

		Comment->Text = *("Missing Node Class " + Key.Key);
		Comment->CommentColor = FLinearColor(1.0, 0.0, 0.0);
		Comment->bCommentBubbleVisible = true;
		Comment->SizeX = 415;
		Comment->SizeY = 40;

		Comment->Desc = "A node is missing in your Unreal Engine build, this may be for many reasons, primarily due to your version of Unreal being younger than the one your porting from.";

		GetObjectSerializer()->DeserializeObjectProperties(Properties, Comment);

		if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) FuncCasted->GetExpressionCollection().AddComment(Comment);
		else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) MatCasted->GetExpressionCollection().AddComment(Comment);
	}
}

void UMaterialGraph_Interface::MaterialGraphNode_ExpressionWrapper(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json) {
	if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) Expression->Function = FuncCasted;
	else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) Expression->Material = MatCasted;

	if (UMaterialExpressionTextureBase* TextureBase = Cast<UMaterialExpressionTextureBase>(Expression))
		if (const TSharedPtr<FJsonObject>* TexturePtr; Json->TryGetObjectField("Texture", TexturePtr)) {
			LoadObject(TexturePtr, TextureBase->Texture);

			Expression->UpdateParameterGuid(true, false);
		}
}

UMaterialExpression* UMaterialGraph_Interface::CreateEmptyExpression(UObject* Parent, FName Name, FName Type, FJsonObject* LocalizedObject) {
	if (IgnoredExpressions.Contains(Type.ToString())) // Unhandled expressions
		return nullptr;

	const UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Type.ToString());

	if (!Class) {
		TArray<FString> Redirects = TArray{
			FLinkerLoad::FindNewPathNameForClass("/Script/InterchangeImport." + Type.ToString(), false),
			FLinkerLoad::FindNewPathNameForClass("/Script/Landscape." + Type.ToString(), false)
		};
		
		for (FString RedirectedPath : Redirects) {
			if (!RedirectedPath.IsEmpty() && !Class)
				Class = FindObject<UClass>(nullptr, *RedirectedPath);
		}

		if (!Class) 
			Class = FindObject<UClass>(ANY_PACKAGE, *Type.ToString().Replace(TEXT("MaterialExpressionPhysicalMaterialOutput"), TEXT("MaterialExpressionLandscapePhysicalMaterialOutput")));
	}

	// Show missing nodes in graph
	if (!Class) {
		TSharedPtr<FJsonObject>* ShareObject = new TSharedPtr<FJsonObject>(LocalizedObject);
		MissingNodeClasses.Add(Type.ToString(), ShareObject->Get());

		GLog->Log(*("JsonAsAsset: Missing Node " + Type.ToString() + " in Parent " + Parent->GetName()));
		FNotificationInfo Info = FNotificationInfo(FText::FromString("Missing Node (" + Parent->GetName() + ")"));

		Info.bUseLargeFont = false;
		Info.bFireAndForget = false;
		Info.FadeOutDuration = .5f;
		Info.ExpireDuration = 8.0f;
		Info.WidthOverride = FOptionalSize(456);
		Info.bUseThrobber = false;
		Info.SubText = FText::FromString(Type.ToString());

#pragma warning(disable: 4800)
		Info.Image = FSlateIconFinder::FindCustomIconBrushForClass(FindObject<UClass>(nullptr, "/Script/Engine.Material"), TEXT("ClassThumbnail"));

		MaterialGraphNotification = FSlateNotificationManager::Get().AddNotification(Info);
		MaterialGraphNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);

		return NewObject<UMaterialExpression>(
			Parent,
			UMaterialExpressionReroute::StaticClass(),
			Name
		);
	}

	return NewObject<UMaterialExpression>
	(
		Parent,
		Class, // Find class using ANY_PACKAGE (may error in the future)
		Name,
		RF_Transactional
	);
}

FExpressionInput UMaterialGraph_Interface::PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type) {
	FExpressionInput Input;
	Input.Expression = Expression;

	// Each Mask input/output
	int OutputIndex;
	if (JsonProperties->TryGetNumberField("OutputIndex", OutputIndex)) Input.OutputIndex = OutputIndex;
	FString InputName;
	if (JsonProperties->TryGetStringField("InputName", InputName)) Input.InputName = FName(InputName);
	int Mask;
	if (JsonProperties->TryGetNumberField("Mask", Mask)) Input.Mask = Mask;
	int MaskR;
	if (JsonProperties->TryGetNumberField("MaskR", MaskR)) Input.MaskR = MaskR;
	int MaskG;
	if (JsonProperties->TryGetNumberField("MaskG", MaskG)) Input.MaskG = MaskG;
	int MaskB;
	if (JsonProperties->TryGetNumberField("MaskB", MaskB)) Input.MaskB = MaskB;
	int MaskA;
	if (JsonProperties->TryGetNumberField("MaskA", MaskA)) Input.MaskA = MaskA;

	if (Type == "Color") {
		if (FColorMaterialInput* ColorInput = reinterpret_cast<FColorMaterialInput*>(&Input)) {
			bool UseConstant;
			if (JsonProperties->TryGetBoolField("UseConstant", UseConstant)) ColorInput->UseConstant = UseConstant;
			const TSharedPtr<FJsonObject>* Constant;
			if (JsonProperties->TryGetObjectField("Constant", Constant)) ColorInput->Constant = FMathUtilities::ObjectToLinearColor(Constant->Get()).ToFColor(true);
			Input = FExpressionInput(*ColorInput);
		}
	} else if (Type == "Scalar") {
		if (FScalarMaterialInput* ScalarInput = reinterpret_cast<FScalarMaterialInput*>(&Input)) {
			bool UseConstant;
			if (JsonProperties->TryGetBoolField("UseConstant", UseConstant)) ScalarInput->UseConstant = UseConstant;
			float Constant;
			if (JsonProperties->TryGetNumberField("Constant", Constant)) ScalarInput->Constant = Constant;
			Input = FExpressionInput(*ScalarInput);
		}
	} else if (Type == "Vector") {
		if (FVectorMaterialInput* VectorInput = reinterpret_cast<FVectorMaterialInput*>(&Input)) {
			bool UseConstant;
			if (JsonProperties->TryGetBoolField("UseConstant", UseConstant)) VectorInput->UseConstant = UseConstant;
			const TSharedPtr<FJsonObject>* Constant;
			if (JsonProperties->TryGetObjectField("Constant", Constant)) VectorInput->Constant = FMathUtilities::ObjectToVector3f(Constant->Get());
			Input = FExpressionInput(*VectorInput);
		}
	}

	return Input;
}

FExpressionOutput UMaterialGraph_Interface::PopulateExpressionOutput(const FJsonObject* JsonProperties) {
	FExpressionOutput Output;

	FString OutputName;
	if (JsonProperties->TryGetStringField("OutputName", OutputName)) Output.OutputName = FName(OutputName);
	int Mask;
	if (JsonProperties->TryGetNumberField("Mask", Mask)) Output.Mask = Mask;
	int MaskR;
	if (JsonProperties->TryGetNumberField("MaskR", MaskR)) Output.MaskR = MaskR;
	int MaskG;
	if (JsonProperties->TryGetNumberField("MaskG", MaskG)) Output.MaskG = MaskG;
	int MaskB;
	if (JsonProperties->TryGetNumberField("MaskB", MaskB)) Output.MaskB = MaskB;
	int MaskA;
	if (JsonProperties->TryGetNumberField("MaskA", MaskA)) Output.MaskA = MaskA;

	return Output;
}

FName UMaterialGraph_Interface::GetExpressionName(const FJsonObject* JsonProperties, FString OverrideParameterName) {
	const TSharedPtr<FJsonValue> ExpressionField = JsonProperties->TryGetField(OverrideParameterName);

	if (ExpressionField == nullptr || ExpressionField->IsNull()) {
		// Must be from < 4.25
		return FName(JsonProperties->GetStringField("ExpressionName"));
	}

	const TSharedPtr<FJsonObject> ExpressionObject = ExpressionField->AsObject();
	FString ObjectName;
	if (ExpressionObject->TryGetStringField("ObjectName", ObjectName)) {
		return GetExportNameOfSubobject(ObjectName);
	}

	return NAME_None;
}
