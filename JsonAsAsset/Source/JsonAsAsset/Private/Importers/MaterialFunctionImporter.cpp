// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstantBiasScale.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Utilities/MathUtilities.h"

bool UMaterialFunctionImporter::ImportData() {
	try {
		// Create Material Function Factory (factory automatically creates the MF)
		UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));
		MaterialFunction->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));
		// Handle edit changes, and add it to the content browser
		HandleAssetCreation(MaterialFunction);

		// Clear any default expressions the engine adds (ex: Result)
		MaterialFunction->GetExpressionCollection().Empty();

		// Define editor only data from the JSON
		FJsonObject* EditorOnlyData = nullptr;
		TMap<FName, TTuple<FName, FJsonObject*>> Exports;

		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject>* SharedObject = new TSharedPtr(Value->AsObject());
			FJsonObject* Object = SharedObject->Get();

			FString Type = Object->GetStringField("Type");
			FString Name = Object->GetStringField("Name");

			if (Type == "MaterialFunctionEditorOnlyData") {
				EditorOnlyData = Object;
				continue;
			}

			TTuple<FName, FJsonObject*> Tuple;
			Tuple.Key = FName(Type);
			Tuple.Value = Object;
			
			Exports.Add(FName(Name), Tuple);
		}

		// Find each node expression
		const TSharedPtr<FJsonObject> StringExpressionCollection = EditorOnlyData->GetObjectField("Properties")->GetObjectField("ExpressionCollection");

		TArray<FName> ExpressionNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressions;
		if (StringExpressionCollection->TryGetArrayField("Expressions", StringExpressions)) {
			for (const TSharedPtr<FJsonValue> Expression : *StringExpressions) {
				ExpressionNames.Add(GetExportNameOfSubobject(Expression->AsObject()->GetStringField("ObjectName")));
			}
		}

		// Map out each experssion for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap;

		for (FName Name : ExpressionNames) {
			FName Type;
			TSet<FName> Keys;
			Exports.GetKeys(Keys);

			for (TTuple<FName, TTuple<FName, FJsonObject*>>& Key : Exports)
				if (Key.Key == Name) {
					Type = FName(Key.Value.Key);
					break;
				}

			UMaterialExpression* Ex = CreateEmptyExpression(MaterialFunction, Name, Type);
			if (Ex == nullptr)
				continue;

			CreatedExpressionMap.Add(Name, Ex);
		}

		// Iterate through all the expression names
		for (FName Name : ExpressionNames) {
			TTuple<FName, FJsonObject*>* Type = Exports.Find(Name);

			const TSharedPtr<FJsonObject> Properties = Type->Value->GetObjectField("Properties");

			// Find the expression from FName
			if (!CreatedExpressionMap.Contains(Name)) continue;
			UMaterialExpression* Expression = *CreatedExpressionMap.Find(Name);

			// Cheeky things here
			if (Type->Key == "MaterialExpressionFunctionOutput") {
				UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(Expression);

				TSharedPtr<FJsonObject> StringA = Properties->GetObjectField("A");
				FName AExpressionName = GetExportNameOfSubobject(StringA->GetObjectField("Expression")->GetStringField("ObjectName"));

				if (CreatedExpressionMap.Contains(AExpressionName)) {
					FExpressionInput A = PopulateExpressionInput(StringA, *CreatedExpressionMap.Find(AExpressionName));
					FunctionOutput->A = A;
				}

				bool bLastPreviewed;
				if (Properties->TryGetBoolField("bLastPreviewed", bLastPreviewed)) FunctionOutput->bLastPreviewed = bLastPreviewed;
				FunctionOutput->Id = FGuid(Properties->GetStringField("ID"));

				Expression = FunctionOutput;
			} else if (Type->Key == "MaterialExpressionStaticSwitchParameter") {
				UMaterialExpressionStaticSwitchParameter* StaticSwitchParameter = Cast<UMaterialExpressionStaticSwitchParameter>(Expression);

				const TSharedPtr<FJsonObject>* APtr;
				if (Properties->TryGetObjectField("A", APtr)) {
					TSharedPtr<FJsonObject> AObject = TSharedPtr<FJsonObject>(APtr->Get());
					FName AExpressionName = GetExportNameOfSubobject(AObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						StaticSwitchParameter->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr;
				if (Properties->TryGetObjectField("B", BPtr)) {
					TSharedPtr<FJsonObject> BObject = TSharedPtr<FJsonObject>(BPtr->Get());
					FName BExpressionName = GetExportNameOfSubobject(BObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						StaticSwitchParameter->B = B;
					}
				}

				FString ExpressionGUID;
				if (Properties->TryGetStringField("ExpressionGUID", ExpressionGUID)) StaticSwitchParameter->ExpressionGUID = FGuid(ExpressionGUID);
				FString ParameterName;
				if (Properties->TryGetStringField("ParameterName", ParameterName)) StaticSwitchParameter->ParameterName = FName(ParameterName);

				Expression = StaticSwitchParameter;
			} else if (Type->Key == "MaterialExpressionFunctionInput") {
				UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>(Expression);

				const TSharedPtr<FJsonObject>* PreviewPtr;
				if (Properties->TryGetObjectField("Preview", PreviewPtr)) {
					TSharedPtr<FJsonObject> PreviewObject = TSharedPtr<FJsonObject>(PreviewPtr->Get());
					FName PreviewExpressionName = GetExportNameOfSubobject(PreviewObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(PreviewExpressionName)) {
						FExpressionInput Preview = PopulateExpressionInput(PreviewObject, *CreatedExpressionMap.Find(PreviewExpressionName));
						FunctionInput->Preview = Preview;
					}
				}

				FString InputName;
				if (Properties->TryGetStringField("InputName", InputName)) FunctionInput->InputName = FName(InputName);
				FunctionInput->Id = FGuid(Properties->GetStringField("ID"));

				bool bUsePreviewValueAsDefault;
				if (Properties->TryGetBoolField("bUsePreviewValueAsDefault", bUsePreviewValueAsDefault)) FunctionInput->bUsePreviewValueAsDefault = bUsePreviewValueAsDefault;
				int SortPriority;
				if (Properties->TryGetNumberField("SortPriority", SortPriority)) FunctionInput->SortPriority = SortPriority;

				Expression = FunctionInput;
			} else if (Type->Key == "MaterialExpressionAbs") {
				UMaterialExpressionAbs* Abs = Cast<UMaterialExpressionAbs>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr;
				if (Properties->TryGetObjectField("Input", InputPtr)) {
					TSharedPtr<FJsonObject> InputObject = TSharedPtr<FJsonObject>(InputPtr->Get());
					FName InputExpressionName = GetExportNameOfSubobject(InputObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Abs->Input = Input;
					}
				}

				Expression = Abs;
			} else if (Type->Key == "MaterialExpressionFrac") {
				UMaterialExpressionFrac* Frac = Cast<UMaterialExpressionFrac>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr;
				if (Properties->TryGetObjectField("Input", InputPtr)) {
					TSharedPtr<FJsonObject> InputObject = TSharedPtr<FJsonObject>(InputPtr->Get());
					FName InputExpressionName = GetExportNameOfSubobject(InputObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Frac->Input = Input;
					}
				}

				Expression = Frac;
			} else if (Type->Key == "MaterialExpressionConstant") {
				UMaterialExpressionConstant* Constant = Cast<UMaterialExpressionConstant>(Expression);

				float R;
				if (Properties->TryGetNumberField("R", R)) Constant->R = R;

				Expression = Constant;
			} else if (Type->Key == "MaterialExpressionAdd") {
				UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(Expression);

				const TSharedPtr<FJsonObject>* APtr;
				if (Properties->TryGetObjectField("A", APtr)) {
					TSharedPtr<FJsonObject> AObject = TSharedPtr<FJsonObject>(APtr->Get());
					FName AExpressionName = GetExportNameOfSubobject(AObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Add->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr;
				if (Properties->TryGetObjectField("B", BPtr)) {
					TSharedPtr<FJsonObject> BObject = TSharedPtr<FJsonObject>(BPtr->Get());
					FName BExpressionName = GetExportNameOfSubobject(BObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Add->B = B;
					}
				}

				float ConstA;
				if (Properties->TryGetNumberField("ConstA", ConstA)) Add->ConstA = ConstA;
				float ConstB;
				if (Properties->TryGetNumberField("ConstB", ConstB)) Add->ConstB = ConstB;

				Expression = Add;
			} else if (Type->Key == "MaterialExpressionLinearInterpolate") {
				UMaterialExpressionLinearInterpolate* LinearInterpolate = Cast<UMaterialExpressionLinearInterpolate>(Expression);

				const TSharedPtr<FJsonObject>* APtr;
				if (Properties->TryGetObjectField("A", APtr)) {
					TSharedPtr<FJsonObject> AObject = TSharedPtr<FJsonObject>(APtr->Get());
					FName AExpressionName = GetExportNameOfSubobject(AObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						LinearInterpolate->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr;
				if (Properties->TryGetObjectField("B", BPtr)) {
					TSharedPtr<FJsonObject> BObject = TSharedPtr<FJsonObject>(BPtr->Get());
					FName BExpressionName = GetExportNameOfSubobject(BObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						LinearInterpolate->B = B;
					}
				}

				const TSharedPtr<FJsonObject>* AlphaPtr;
				if (Properties->TryGetObjectField("Alpha", AlphaPtr)) {
					TSharedPtr<FJsonObject> AlphaObject = TSharedPtr<FJsonObject>(BPtr->Get());
					FName AlphaExpressionName = GetExportNameOfSubobject(AlphaPtr->Get()->GetObjectField("Expression")->GetStringField("ObjectName"));
					if (CreatedExpressionMap.Contains(AlphaExpressionName)) {
						FExpressionInput Alpha = PopulateExpressionInput(AlphaObject, *CreatedExpressionMap.Find(AlphaExpressionName));
						LinearInterpolate->Alpha = Alpha;
					}
				}

				float ConstA;
				if (Properties->TryGetNumberField("ConstA", ConstA)) LinearInterpolate->ConstA = ConstA;
				float ConstB;
				if (Properties->TryGetNumberField("ConstB", ConstB)) LinearInterpolate->ConstB = ConstB;
				float ConstAlpha;
				if (Properties->TryGetNumberField("ConstAlpha", ConstAlpha)) LinearInterpolate->ConstAlpha = ConstAlpha;

				Expression = LinearInterpolate;
			} else if (Type->Key == "MaterialExpressionComponentMask") {
				UMaterialExpressionComponentMask* ComponentMask = Cast<UMaterialExpressionComponentMask>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr;
				if (Properties->TryGetObjectField("Input", InputPtr)) {
					TSharedPtr<FJsonObject> InputObject = TSharedPtr<FJsonObject>(InputPtr->Get());
					FName InputExpressionName = GetExportNameOfSubobject(InputObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						ComponentMask->Input = Input;
					}
				}

				bool R;
				if (Properties->TryGetBoolField("R", R)) ComponentMask->R = R;
				bool G;
				if (Properties->TryGetBoolField("G", G)) ComponentMask->G = G;
				bool B;
				if (Properties->TryGetBoolField("B", B)) ComponentMask->B = B;

				Expression = ComponentMask;
			} else if (Type->Key == "MaterialExpressionConstant2Vector") {
				UMaterialExpressionConstant2Vector* Vector2 = Cast<UMaterialExpressionConstant2Vector>(Expression);

				float R;
				if (Properties->TryGetNumberField("R", R)) Vector2->R = R;
				float G;
				if (Properties->TryGetNumberField("G", G)) Vector2->G = G;

				Expression = Vector2;
			} else if (Type->Key == "MaterialExpressionConstant3Vector") {
				UMaterialExpressionConstant3Vector* Vector3 = Cast<UMaterialExpressionConstant3Vector>(Expression);

				const TSharedPtr<FJsonObject>* Constant;
				if (Properties->TryGetObjectField("Constant", Constant)) Vector3->Constant = FMathUtilities::ObjectToLinearColor(Constant->Get());

				Expression = Vector3;
			} else if (Type->Key == "MaterialExpressionConstantBiasScale") {
				UMaterialExpressionConstantBiasScale* ConstantBiasScale = Cast<UMaterialExpressionConstantBiasScale>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr;
				if (Properties->TryGetObjectField("Input", InputPtr)) {
					TSharedPtr<FJsonObject> InputObject = TSharedPtr<FJsonObject>(InputPtr->Get());
					FName InputExpressionName = GetExportNameOfSubobject(InputObject->GetObjectField("Expression")->GetStringField("ObjectName"));

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						ConstantBiasScale->Input = Input;
					}
				}

				float Bias;
				if (Properties->TryGetNumberField("Bias", Bias)) ConstantBiasScale->Bias = Bias;
				float Scale;
				if (Properties->TryGetNumberField("Scale", Scale)) ConstantBiasScale->Scale = Scale;

				Expression = ConstantBiasScale;
			}

			AddGenerics(MaterialFunction, Expression, Properties);
			MaterialFunction->GetExpressionCollection().AddExpression(Expression);
		}

		TArray<FString> ExpressionCommentNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments;
		if (StringExpressionCollection->TryGetArrayField("EditorComments", StringExpressionComments)) {
			for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
				FName ExportName = GetExportNameOfSubobject(ExpressionComment.Get()->AsObject()->GetStringField("ObjectName"));

				const TSharedPtr<FJsonObject> Comment = Exports.Find(ExportName)->Value->GetObjectField("Properties");
				UMaterialExpressionComment* MatComment = NewObject<UMaterialExpressionComment>(MaterialFunction, UMaterialExpressionComment::StaticClass(), ExportName, RF_Transactional);

				int SizeX;
				int SizeY;
				int FontSize;

				FString Text;

				const TSharedPtr<FJsonObject>* CommentColor;

				if (Comment->TryGetNumberField("SizeX", SizeX)) MatComment->SizeX = SizeX;
				if (Comment->TryGetNumberField("SizeY", SizeY)) MatComment->SizeY = SizeY;
				if (Comment->TryGetStringField("Text", Text)) MatComment->Text = Text;
				if (Comment->TryGetObjectField("CommentColor", CommentColor)) MatComment->CommentColor = FMathUtilities::ObjectToLinearColor(CommentColor->Get());
				if (Comment->TryGetNumberField("FontSize", FontSize)) MatComment->FontSize = FontSize;
				AddGenerics(MaterialFunction, MatComment, Comment);

				MaterialFunction->GetExpressionCollection().AddComment(MatComment);
			}
		}
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception))
		return false;
	}

	return true;
}

void UMaterialFunctionImporter::AddGenerics(UMaterialFunction* MaterialFunction, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json) {
	int MaterialExpressionEditorX;
	int MaterialExpressionEditorY;
	bool bCollapsed;

	if (Json->TryGetNumberField("MaterialExpressionEditorX", MaterialExpressionEditorX)) Expression->MaterialExpressionEditorX = MaterialExpressionEditorX;
	if (Json->TryGetNumberField("MaterialExpressionEditorY", MaterialExpressionEditorY)) Expression->MaterialExpressionEditorY = MaterialExpressionEditorY;
	Expression->MaterialExpressionGuid = FGuid(Json->GetStringField("MaterialExpressionGuid"));
	Expression->Function = MaterialFunction;
	if (Json->TryGetBoolField("bCollapsed", bCollapsed)) Expression->bCollapsed = bCollapsed;
}

UMaterialExpression* UMaterialFunctionImporter::CreateEmptyExpression(UMaterialFunction* Parent, const FName Name, const FName Type) const {
	UMaterialExpression* Expression = nullptr;

	if (Type == "MaterialExpressionAbs") Expression = NewObject<UMaterialExpressionAbs>(Parent, UMaterialExpressionAbs::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionAdd") Expression = NewObject<UMaterialExpressionAdd>(Parent, UMaterialExpressionAdd::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionStaticSwitchParameter") Expression = NewObject<UMaterialExpressionStaticSwitchParameter>(Parent, UMaterialExpressionStaticSwitchParameter::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionFunctionOutput") Expression = NewObject<UMaterialExpressionFunctionOutput>(Parent, UMaterialExpressionFunctionOutput::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionFunctionInput") Expression = NewObject<UMaterialExpressionFunctionInput>(Parent, UMaterialExpressionFunctionInput::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionComponentMask") Expression = NewObject<UMaterialExpressionComponentMask>(Parent, UMaterialExpressionComponentMask::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionConstant") Expression = NewObject<UMaterialExpressionConstant>(Parent, UMaterialExpressionConstant::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionConstant2Vector") Expression = NewObject<UMaterialExpressionConstant2Vector>(Parent, UMaterialExpressionConstant2Vector::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionConstant3Vector") Expression = NewObject<UMaterialExpressionConstant3Vector>(Parent, UMaterialExpressionConstant3Vector::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionConstantBiasScale") Expression = NewObject<UMaterialExpressionConstantBiasScale>(Parent, UMaterialExpressionConstantBiasScale::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionFrac") Expression = NewObject<UMaterialExpressionFrac>(Parent, UMaterialExpressionFrac::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionLinearInterpolate") Expression = NewObject<UMaterialExpressionLinearInterpolate>(Parent, UMaterialExpressionLinearInterpolate::StaticClass(), Name, RF_Transactional);

	if (!Expression) {
		UE_LOG(LogJson, Warning, TEXT("Missing support for expression type: \"%s\""), *Type.ToString())
		return nullptr;
	}

	return Expression;
}

FExpressionInput UMaterialFunctionImporter::PopulateExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, UMaterialExpression* Expression) {
	FExpressionInput Input;
	Input.Expression = Expression;
	Input.OutputIndex = JsonProperties->GetIntegerField("OutputIndex");
	Input.InputName = FName(JsonProperties->GetStringField("InputName"));

	// Each Mask input/output
	int Mask; if (JsonProperties->TryGetNumberField("Mask", Mask)) Input.Mask = Mask;
	int MaskR; if (JsonProperties->TryGetNumberField("MaskR", MaskR)) Input.MaskR = MaskR;
	int MaskG; if (JsonProperties->TryGetNumberField("MaskG", MaskG)) Input.MaskG = MaskG;
	int MaskB; if (JsonProperties->TryGetNumberField("MaskB", MaskB)) Input.MaskB = MaskB;
	int MaskA; if (JsonProperties->TryGetNumberField("MaskA", MaskA)) Input.MaskA = MaskA;

	return Input;
}
