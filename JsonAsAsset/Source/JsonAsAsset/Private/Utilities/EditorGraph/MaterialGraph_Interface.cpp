// Copyright Epic Games, Inc. All Rights Reserved.

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
#include "Materials/MaterialExpressionMakeMaterialAttributes.h"
#include "Materials/MaterialExpressionGetMaterialAttributes.h"
#include "Materials/MaterialExpressionBreakMaterialAttributes.h"
#include "Materials/MaterialExpressionBlendMaterialAttributes.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"

#include <MaterialEditingLibrary.h>
#include <Editor/UnrealEd/Public/FileHelpers.h>

TSharedPtr<FJsonObject> UMaterialGraph_Interface::FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FImportData>& OutExports, TArray<FName>& ExpressionNames, bool bFilterByOuter) {
	TSharedPtr<FJsonObject> EditorOnlyData;

	for (const TSharedPtr<FJsonValue> Value : (bFilterByOuter ? FilterExportsByOuter(Outer) : AllJsonObjects)) {
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
		bool bFound = false;

		for (TTuple<FName, FImportData>& Key : Exports) {
			if (Key.Key == Name && Key.Value.Outer == FName(Outer)) {
				Type = Key.Value.Type;
				bFound = true;
				break;
			}
		}

		if (!bFound) continue;
		UMaterialExpression* Ex = CreateEmptyExpression(Parent, Name, Type);
		if (Ex == nullptr) continue;

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
					if (!HandleReference(ObjectPath)) AppendNotification(FText::FromString("Material Function Missing: " + ObjectPath), FText::FromString("Material Graph"), 2.0f, SNotificationItem::CS_Fail, true);
					else LoadObject(MaterialFunctionPtr, MaterialFunctionCall->MaterialFunction);
				}
			}
		}

		// Sets 99% of properties for nodes
		GetObjectSerializer()->DeserializeObjectProperties(Properties, Expression);

		// ------------ Material Attributes have GUIDs, unsupported for property serializer ------------ 
		if (Type->Type == "MaterialExpressionGetMaterialAttributes") {
			UMaterialExpressionGetMaterialAttributes* GetMaterialAttributes = Cast<UMaterialExpressionGetMaterialAttributes>(Expression);

			GetMaterialAttributes->AttributeGetTypes.Empty();
			if (const TArray<TSharedPtr<FJsonValue>>* AttributeGetTypesPtr; Properties->TryGetArrayField("AttributeGetTypes", AttributeGetTypesPtr)) {
				int i = 0;
				for (const TSharedPtr<FJsonValue> AttributeGetTypeValue : *AttributeGetTypesPtr) {
					FString AttributeGetType = AttributeGetTypeValue->AsString();
					GetMaterialAttributes->AttributeGetTypes.Add(FGuid(AttributeGetType));
					i++;
				}
			}

			if (const TArray<TSharedPtr<FJsonValue>>* OutputsPtr; Properties->TryGetArrayField("Outputs", OutputsPtr)) {
				TArray<FExpressionOutput> Outputs;
				for (const TSharedPtr<FJsonValue> FunctionOutputValue : *OutputsPtr) {
					Outputs.Add(PopulateExpressionOutput(FunctionOutputValue->AsObject().Get()));
				}

				GetMaterialAttributes->Outputs = Outputs;
			}
		} else if (Type->Type == "MaterialExpressionSetMaterialAttributes") {
			UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = Cast<UMaterialExpressionSetMaterialAttributes>(Expression);

			if (const TArray<TSharedPtr<FJsonValue>>* InputsPtr; Properties->TryGetArrayField("Inputs", InputsPtr)) {
				int i = 0;
				SetMaterialAttributes->Inputs.Empty();
				SetMaterialAttributes->Inputs.SetNumZeroed(InputsPtr->Num());
				for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
					FJsonObject* InputObject = InputValue->AsObject().Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						SetMaterialAttributes->Inputs[i] = Input;
					}
					i++;
				}
			}

			if (const TArray<TSharedPtr<FJsonValue>>* AttributeSetTypesPtr; Properties->TryGetArrayField("AttributeSetTypes", AttributeSetTypesPtr)) {
				int i = 0;
				SetMaterialAttributes->AttributeSetTypes.Empty();
				SetMaterialAttributes->AttributeSetTypes.SetNumZeroed(AttributeSetTypesPtr->Num());
				for (const TSharedPtr<FJsonValue> AttributeSetTypeValue : *AttributeSetTypesPtr) {
					FString AttributeSetType = AttributeSetTypeValue->AsString();
					SetMaterialAttributes->AttributeSetTypes[i] = FGuid(AttributeSetType);
					i++;
				}
			}
		}
		
		// Properties with incorrect letter cases (and GUID problems)
		if (Type->Type == "MaterialExpressionFunctionOutput") {
			UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(Expression);
				FunctionOutput->Id = FGuid(Properties->GetStringField("ID"));
		} else if (Type->Type == "MaterialExpressionFunctionInput") {
			UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>(Expression);
				FunctionInput->Id = FGuid(Properties->GetStringField("ID"));
		}

		// Material Function Manually: To define function inputs correctly?
		if (Type->Type == "MaterialExpressionMaterialFunctionCall") {
			UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);

			if (const TArray<TSharedPtr<FJsonValue>>* FunctionInputsPtr; Properties->TryGetArrayField("FunctionInputs", FunctionInputsPtr)) {
				TArray<FFunctionExpressionInput> FunctionInputs;

				for (const TSharedPtr<FJsonValue> FunctionInputValue : *FunctionInputsPtr)
					FunctionInputs.Add(PopulateFuncExpressionInput(FunctionInputValue->AsObject(), CreatedExpressionMap));

				MaterialFunctionCall->FunctionInputs = FunctionInputs;
			}

			if (const TArray<TSharedPtr<FJsonValue>>* FunctionOutputsPtr; Properties->TryGetArrayField("FunctionOutputs", FunctionOutputsPtr)) {
				TArray<FFunctionExpressionOutput> FunctionOutputs;

				for (const TSharedPtr<FJsonValue> FunctionOutputValue : *FunctionOutputsPtr)
					FunctionOutputs.Add(PopulateFuncExpressionOutput(FunctionOutputValue->AsObject()));

				MaterialFunctionCall->FunctionOutputs = FunctionOutputs;
			}
		}

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

UMaterialExpression* UMaterialGraph_Interface::CreateEmptyExpression(UObject* Parent, const FName Name, const FName Type) const {
	if (IgnoredExpressions.Contains(Type.ToString())) // Unhandled expressions
		return nullptr;

	return NewObject<UMaterialExpression>
		(
			Parent, 
			FindObjectChecked<UClass>(ANY_PACKAGE, *Type.ToString()), // Find class using ANY_PACKAGE (may error in the future)
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

FFunctionExpressionOutput UMaterialGraph_Interface::PopulateFuncExpressionOutput(const TSharedPtr<FJsonObject>& JsonProperties) {
	FFunctionExpressionOutput Output;

	FString ExpressionOutputId;
	if (JsonProperties->TryGetStringField("ExpressionOutputId", ExpressionOutputId)) Output.ExpressionOutputId = FGuid(ExpressionOutputId);

	const TSharedPtr<FJsonObject>* OutputPtr = nullptr;
	if (JsonProperties->TryGetObjectField("Output", OutputPtr) && OutputPtr != nullptr) {
		Output.Output = PopulateExpressionOutput(OutputPtr->Get());
	}

	return Output;
}

FFunctionExpressionInput UMaterialGraph_Interface::PopulateFuncExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap) {
	FFunctionExpressionInput Input;

	FString ExpressionInputId;
	if (JsonProperties->TryGetStringField("ExpressionInputId", ExpressionInputId)) Input.ExpressionInputId = FGuid(ExpressionInputId);

	const TSharedPtr<FJsonObject>* InputPtr = nullptr;
	if (JsonProperties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
		const FJsonObject* InputObject = InputPtr->Get();
		const FName InputExpressionName = GetExpressionName(InputObject);

		if (CreatedExpressionMap.Contains(InputExpressionName)) {
			const FExpressionInput ExInput = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
			Input.Input = ExInput;
		}
	}

	return Input;
}
