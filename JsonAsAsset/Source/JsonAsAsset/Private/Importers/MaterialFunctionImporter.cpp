// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"

#include "Dom/JsonObject.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstantBiasScale.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionMin.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionReroute.h"
#include "Materials/MaterialExpressionRotator.h"
#include "Materials/MaterialExpressionRound.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionStep.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Utilities/MathUtilities.h"

struct FTestImportData {
	FTestImportData(const FName Type, const TSharedPtr<FJsonObject>& Json) {
		this->Type = Type;
		this->Json = Json.Get();
	}

	FTestImportData(const FString& Type, const TSharedPtr<FJsonObject>& Json) {
		this->Type = FName(Type);
		this->Json = Json.Get();
	}

	FTestImportData(const FString& Type, FJsonObject* Json) {
		this->Type = FName(Type);
		this->Json = Json;
	}

	FName Type;
	FJsonObject* Json;
};

bool UMaterialFunctionImporter::ImportData() {
	try {
		// Create Material Function Factory (factory automatically creates the MF)
		UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

		MaterialFunction->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));
		FString Description;
		if (JsonObject->GetObjectField("Properties")->TryGetStringField("Description", Description)) MaterialFunction->Description = Description;
		bool bExposeToLibrary;
		if (JsonObject->GetObjectField("Properties")->TryGetBoolField("bExposeToLibrary", bExposeToLibrary)) MaterialFunction->bExposeToLibrary = bExposeToLibrary;

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(MaterialFunction)) return false;

		// Clear any default expressions the engine adds (ex: Result)
		MaterialFunction->GetExpressionCollection().Empty();

		// Define editor only data from the JSON
		TSharedPtr<FJsonObject> EditorOnlyData = nullptr;
		TMap<FName, FTestImportData> Exports;

		for (const TSharedPtr<FJsonValue> Value : AllJsonObjects) {
			TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

			FString Type = Object->GetStringField("Type");
			FString Name = Object->GetStringField("Name");

			if (Type == "MaterialFunctionEditorOnlyData") {
				EditorOnlyData = Object;
				continue;
			}

			Exports.Add(FName(Name), FTestImportData(Type, Object));
		}

		// Find each node expression
		const TSharedPtr<FJsonObject> StringExpressionCollection = EditorOnlyData->GetObjectField("Properties")->GetObjectField("ExpressionCollection");

		TArray<FName> ExpressionNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressions;
		if (StringExpressionCollection->TryGetArrayField("Expressions", StringExpressions)) {
			for (const TSharedPtr<FJsonValue> Expression : *StringExpressions) {
				if (Expression->IsNull()) continue;
				ExpressionNames.Add(GetExportNameOfSubobject(Expression->AsObject()->GetStringField("ObjectName")));
			}
		}

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap;

		for (FName Name : ExpressionNames) {
			FName Type;
			TSet<FName> Keys;
			Exports.GetKeys(Keys);

			for (TTuple<FName, FTestImportData> Key : Exports) {
				if (Key.Key == Name) {
					Type = Key.Value.Type;
					break;
				}
			}

			UMaterialExpression* Ex = CreateEmptyExpression(MaterialFunction, Name, Type);
			if (Ex == nullptr)
				continue;

			CreatedExpressionMap.Add(Name, Ex);
		}

		// Iterate through all the expression names
		for (FName Name : ExpressionNames) {
			FTestImportData* Type = Exports.Find(Name);

			TSharedPtr<FJsonObject> Properties = Type->Json->GetObjectField("Properties");

			// Find the expression from FName
			if (!CreatedExpressionMap.Contains(Name)) continue;
			UMaterialExpression* Expression = *CreatedExpressionMap.Find(Name);

			// Cheeky things here
			if (Type->Type == "MaterialExpressionFunctionOutput") {
				UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(Expression);

				FString OutputName;
				if (Properties->TryGetStringField("OutputName", OutputName)) FunctionOutput->OutputName = FName(OutputName);
				FString FuncDescription;
				if (Properties->TryGetStringField("Description", FuncDescription)) FunctionOutput->Description = FuncDescription;
				int SortPriority;
				if (Properties->TryGetNumberField("SortPriority", SortPriority)) FunctionOutput->SortPriority = SortPriority;

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);

					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						FunctionOutput->A = A;
					}
				}

				bool bLastPreviewed;
				if (Properties->TryGetBoolField("bLastPreviewed", bLastPreviewed)) FunctionOutput->bLastPreviewed = bLastPreviewed;
				FunctionOutput->Id = FGuid(Properties->GetStringField("ID"));

				Expression = FunctionOutput;
			} else if (Type->Type == "MaterialExpressionStaticSwitchParameter") {
				UMaterialExpressionStaticSwitchParameter* StaticSwitchParameter = Cast<UMaterialExpressionStaticSwitchParameter>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);

					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						StaticSwitchParameter->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);

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
			} else if (Type->Type == "MaterialExpressionFunctionInput") {
				UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>(Expression);

				const TSharedPtr<FJsonObject>* PreviewPtr = nullptr;
				if (Properties->TryGetObjectField("Preview", PreviewPtr) && PreviewPtr != nullptr) {
					FJsonObject* PreviewObject = PreviewPtr->Get();
					FName PreviewExpressionName = GetExpressionName(PreviewObject);

					if (CreatedExpressionMap.Contains(PreviewExpressionName)) {
						FExpressionInput Preview = PopulateExpressionInput(PreviewObject, *CreatedExpressionMap.Find(PreviewExpressionName));
						FunctionInput->Preview = Preview;
					}
				}

				FString InputName;
				if (Properties->TryGetStringField("InputName", InputName)) FunctionInput->InputName = FName(InputName);
				FString FuncDescription;
				if (Properties->TryGetStringField("Description", FuncDescription)) FunctionInput->Description = FuncDescription;
				FunctionInput->Id = FGuid(Properties->GetStringField("ID"));

				FString InputTypeString;
				if (Properties->TryGetStringField("InputType", InputTypeString)) {
					EFunctionInputType InputType;
					if (InputTypeString.EndsWith("FunctionInput_Scalar")) InputType = FunctionInput_Scalar;
					else if (InputTypeString.EndsWith("FunctionInput_Vector2")) InputType = FunctionInput_Vector2;
					else if (InputTypeString.EndsWith("FunctionInput_Vector4")) InputType = FunctionInput_Vector4;
					else if (InputTypeString.EndsWith("FunctionInput_Texture2D")) InputType = FunctionInput_Texture2D;
					else if (InputTypeString.EndsWith("FunctionInput_TextureCube")) InputType = FunctionInput_TextureCube;
					else if (InputTypeString.EndsWith("FunctionInput_Texture2DArray")) InputType = FunctionInput_Texture2DArray;
					else if (InputTypeString.EndsWith("FunctionInput_VolumeTexture")) InputType = FunctionInput_VolumeTexture;
					else if (InputTypeString.EndsWith("FunctionInput_StaticBool")) InputType = FunctionInput_StaticBool;
					else if (InputTypeString.EndsWith("FunctionInput_MaterialAttributes")) InputType = FunctionInput_MaterialAttributes;
					else if (InputTypeString.EndsWith("FunctionInput_TextureExternal")) InputType = FunctionInput_TextureExternal;
					else if (InputTypeString.EndsWith("FunctionInput_Bool")) InputType = FunctionInput_Bool;
					else if (InputTypeString.EndsWith("FunctionInput_Substrate")) InputType = FunctionInput_Substrate;
					else InputType = FunctionInput_Vector3;

					FunctionInput->InputType = InputType;
				}

				bool bUsePreviewValueAsDefault;
				if (Properties->TryGetBoolField("bUsePreviewValueAsDefault", bUsePreviewValueAsDefault)) FunctionInput->bUsePreviewValueAsDefault = bUsePreviewValueAsDefault;
				int SortPriority;
				if (Properties->TryGetNumberField("SortPriority", SortPriority)) FunctionInput->SortPriority = SortPriority;

				Expression = FunctionInput;
			} else if (Type->Type == "MaterialExpressionAbs") {
				UMaterialExpressionAbs* Abs = Cast<UMaterialExpressionAbs>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Abs->Input = Input;
					}
				}

				Expression = Abs;
			} else if (Type->Type == "MaterialExpressionFrac") {
				UMaterialExpressionFrac* Frac = Cast<UMaterialExpressionFrac>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Frac->Input = Input;
					}
				}

				Expression = Frac;
			} else if (Type->Type == "MaterialExpressionConstant") {
				UMaterialExpressionConstant* Constant = Cast<UMaterialExpressionConstant>(Expression);

				float R;
				if (Properties->TryGetNumberField("R", R)) Constant->R = R;

				Expression = Constant;
			} else if (Type->Type == "MaterialExpressionAdd") {
				UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);

					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Add->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);

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
			} else if (Type->Type == "MaterialExpressionLinearInterpolate") {
				UMaterialExpressionLinearInterpolate* LinearInterpolate = Cast<UMaterialExpressionLinearInterpolate>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						LinearInterpolate->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						LinearInterpolate->B = B;
					}
				}

				const TSharedPtr<FJsonObject>* AlphaPtr = nullptr;
				if (Properties->TryGetObjectField("Alpha", AlphaPtr) && AlphaPtr != nullptr) {
					FJsonObject* AlphaObject = AlphaPtr->Get();
					FName AlphaExpressionName = GetExpressionName(AlphaObject);
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
			} else if (Type->Type == "MaterialExpressionComponentMask") {
				UMaterialExpressionComponentMask* ComponentMask = Cast<UMaterialExpressionComponentMask>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);

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
			} else if (Type->Type == "MaterialExpressionConstant2Vector") {
				UMaterialExpressionConstant2Vector* Vector2 = Cast<UMaterialExpressionConstant2Vector>(Expression);

				float R;
				if (Properties->TryGetNumberField("R", R)) Vector2->R = R;
				float G;
				if (Properties->TryGetNumberField("G", G)) Vector2->G = G;

				Expression = Vector2;
			} else if (Type->Type == "MaterialExpressionConstant3Vector") {
				UMaterialExpressionConstant3Vector* Vector3 = Cast<UMaterialExpressionConstant3Vector>(Expression);

				const TSharedPtr<FJsonObject>* Constant;
				if (Properties->TryGetObjectField("Constant", Constant)) Vector3->Constant = FMathUtilities::ObjectToLinearColor(Constant->Get());

				Expression = Vector3;
			} else if (Type->Type == "MaterialExpressionConstantBiasScale") {
				UMaterialExpressionConstantBiasScale* ConstantBiasScale = Cast<UMaterialExpressionConstantBiasScale>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);

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
			} else if (Type->Type == "MaterialExpressionOneMinus") {
				UMaterialExpressionOneMinus* OneMinus = Cast<UMaterialExpressionOneMinus>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);

					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						OneMinus->Input = Input;
					}
				}

				Expression = OneMinus;
			} else if (Type->Type == "MaterialExpressionMultiply") {
				UMaterialExpressionMultiply* Multiply = Cast<UMaterialExpressionMultiply>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Multiply->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Multiply->B = B;
					}
				}

				float ConstA;
				if (Properties->TryGetNumberField("ConstA", ConstA)) Multiply->ConstA = ConstA;
				float ConstB;
				if (Properties->TryGetNumberField("ConstB", ConstB)) Multiply->ConstB = ConstB;

				Expression = Multiply;
			} else if (Type->Type == "MaterialExpressionVectorParameter") {
				UMaterialExpressionVectorParameter* VectorParameter = Cast<UMaterialExpressionVectorParameter>(Expression);

				const TSharedPtr<FJsonObject>* DefaultValue;
				if (Properties->TryGetObjectField("DefaultValue", DefaultValue)) VectorParameter->DefaultValue = FMathUtilities::ObjectToLinearColor(DefaultValue->Get());

				FString ExpressionGUID;
				if (Properties->TryGetStringField("ExpressionGUID", ExpressionGUID)) VectorParameter->ExpressionGUID = FGuid(ExpressionGUID);
				FString ParameterName;
				if (Properties->TryGetStringField("ParameterName", ParameterName)) VectorParameter->ParameterName = FName(ParameterName);
				FString Group;
				if (Properties->TryGetStringField("Group", Group)) VectorParameter->Group = FName(Group);

				Expression = VectorParameter;
			} else if (Type->Type == "MaterialExpressionTextureSample") {
				UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>(Expression);

				const TSharedPtr<FJsonObject>* CoordinatesPtr = nullptr;
				if (Properties->TryGetObjectField("Coordinates", CoordinatesPtr) && CoordinatesPtr != nullptr) {
					FJsonObject* CoordinatesObject = CoordinatesPtr->Get();
					FName CoordinatesExpressionName = GetExpressionName(CoordinatesObject);
					if (CreatedExpressionMap.Contains(CoordinatesExpressionName)) {
						FExpressionInput Coordinates = PopulateExpressionInput(CoordinatesObject, *CreatedExpressionMap.Find(CoordinatesExpressionName));
						TextureSample->Coordinates = Coordinates;
					}
				}

				const TSharedPtr<FJsonObject>* TextureObjectPtr = nullptr;
				if (Properties->TryGetObjectField("TextureObject", TextureObjectPtr) && TextureObjectPtr != nullptr) {
					FJsonObject* TextureObjectObject = TextureObjectPtr->Get();
					FName TextureObjectExpressionName = GetExpressionName(TextureObjectObject);
					if (CreatedExpressionMap.Contains(TextureObjectExpressionName)) {
						FExpressionInput TextureObject = PopulateExpressionInput(TextureObjectObject, *CreatedExpressionMap.Find(TextureObjectExpressionName));
						TextureSample->TextureObject = TextureObject;
					}
				}

				FString SampleSourceString;
				if (Properties->TryGetStringField("SamplerSource", SampleSourceString)) {
					ESamplerSourceMode SamplerSource;

					if (SampleSourceString.EndsWith("SSM_Wrap_WorldGroupSettings")) SamplerSource = SSM_Wrap_WorldGroupSettings;
					else if (SampleSourceString.EndsWith("SSM_Clamp_WorldGroupSettings")) SamplerSource = SSM_Clamp_WorldGroupSettings;
					else SamplerSource = SSM_FromTextureAsset;

					TextureSample->SamplerSource = SamplerSource;
				}

				const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
				if (Properties->TryGetObjectField("Texture", TexturePtr) && TexturePtr != nullptr) {
					TextureSample->Texture = LoadObject<UTexture>(TexturePtr);
				}

				FString SamplerTypeString;
				if (Properties->TryGetStringField("SamplerType", SamplerTypeString)) {
					EMaterialSamplerType SamplerType;

					if (SamplerTypeString.EndsWith("SAMPLERTYPE_Grayscale")) SamplerType = SAMPLERTYPE_Grayscale;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Alpha")) SamplerType = SAMPLERTYPE_Alpha;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Normal")) SamplerType = SAMPLERTYPE_Normal;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Masks")) SamplerType = SAMPLERTYPE_Masks;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_DistanceFieldFont")) SamplerType = SAMPLERTYPE_DistanceFieldFont;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_LinearColor")) SamplerType = SAMPLERTYPE_LinearColor;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_LinearGrayscale")) SamplerType = SAMPLERTYPE_LinearGrayscale;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Data")) SamplerType = SAMPLERTYPE_Data;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_External")) SamplerType = SAMPLERTYPE_External;
					else SamplerType = SAMPLERTYPE_Color;

					TextureSample->SamplerType = SamplerType;
				}

				Expression = TextureSample;
			} else if (Type->Type == "MaterialExpressionMaterialFunctionCall") {
				UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);

				const TSharedPtr<FJsonObject>* MaterialFunctionPtr = nullptr;
				if (Properties->TryGetObjectField("MaterialFunction", MaterialFunctionPtr) && MaterialFunctionPtr != nullptr) {
					MaterialFunctionCall->MaterialFunction = LoadObject<UMaterialFunction>(MaterialFunctionPtr);
				}

				TArray<FFunctionExpressionInput> FunctionInputs;
				const TArray<TSharedPtr<FJsonValue>>* FunctionInputsPtr;
				if (Properties->TryGetArrayField("FunctionInputs", FunctionInputsPtr)) {
					for (const TSharedPtr<FJsonValue> FunctionInputValue : *FunctionInputsPtr) {
						TSharedPtr<FJsonObject> FunctionInputObject = FunctionInputValue->AsObject();

						FunctionInputs.Add(PopulateFuncExpressionInput(FunctionInputObject, CreatedExpressionMap));
					}
				}

				MaterialFunctionCall->FunctionInputs = FunctionInputs;

				TArray<FFunctionExpressionOutput> FunctionOutputs;
				const TArray<TSharedPtr<FJsonValue>>* FunctionOutputsPtr;
				if (Properties->TryGetArrayField("FunctionOutputs", FunctionOutputsPtr)) {
					for (const TSharedPtr<FJsonValue> FunctionOutputValue : *FunctionOutputsPtr) {
						TSharedPtr<FJsonObject> FunctionOutputObject = FunctionOutputValue->AsObject();

						FunctionOutputs.Add(PopulateFuncExpressionOutput(FunctionOutputObject));
					}
				}

				MaterialFunctionCall->FunctionOutputs = FunctionOutputs;

				TArray<FExpressionOutput> Outputs;
				const TArray<TSharedPtr<FJsonValue>>* OutputsPtr;
				if (Properties->TryGetArrayField("Outputs", OutputsPtr)) {
					for (const TSharedPtr<FJsonValue> FunctionOutputValue : *OutputsPtr) {
						Outputs.Add(PopulateExpressionOutput(FunctionOutputValue->AsObject().Get()));
					}
				}

				MaterialFunctionCall->Outputs = Outputs;

				Expression = MaterialFunctionCall;
			} else if (Type->Type == "MaterialExpressionTextureObject") {
				UMaterialExpressionTextureObject* TextureObject = Cast<UMaterialExpressionTextureObject>(Expression);

				const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
				if (Properties->TryGetObjectField("Texture", TexturePtr) && TexturePtr != nullptr) {
					TextureObject->Texture = LoadObject<UTexture>(TexturePtr);
				}

				FString SamplerTypeString;
				if (Properties->TryGetStringField("SamplerType", SamplerTypeString)) {
					EMaterialSamplerType SamplerType;

					if (SamplerTypeString.EndsWith("SAMPLERTYPE_Grayscale")) SamplerType = SAMPLERTYPE_Grayscale;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Alpha")) SamplerType = SAMPLERTYPE_Alpha;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Normal")) SamplerType = SAMPLERTYPE_Normal;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Masks")) SamplerType = SAMPLERTYPE_Masks;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_DistanceFieldFont")) SamplerType = SAMPLERTYPE_DistanceFieldFont;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_LinearColor")) SamplerType = SAMPLERTYPE_LinearColor;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_LinearGrayscale")) SamplerType = SAMPLERTYPE_LinearGrayscale;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Data")) SamplerType = SAMPLERTYPE_Data;
					else if (SamplerTypeString.EndsWith("SAMPLERTYPE_External")) SamplerType = SAMPLERTYPE_External;
					else SamplerType = SAMPLERTYPE_Color;

					TextureObject->SamplerType = SamplerType;
				}

				Expression = TextureObject;
			} else if (Type->Type == "MaterialExpressionMax") {
				UMaterialExpressionMax* Max = Cast<UMaterialExpressionMax>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Max->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Max->B = B;
					}
				}

				Expression = Max;
			} else if (Type->Type == "MaterialExpressionTextureCoordinate") {
				UMaterialExpressionTextureCoordinate* TextureCoordinate = Cast<UMaterialExpressionTextureCoordinate>(Expression);

				int CoordinateIndex;
				if (Properties->TryGetNumberField("CoordinateIndex", CoordinateIndex)) TextureCoordinate->CoordinateIndex = CoordinateIndex;

				Expression = TextureCoordinate;
			} else if (Type->Type == "MaterialExpressionTime") {
				// No idea
			} else if (Type->Type == "MaterialExpressionScalarParameter") {
				UMaterialExpressionScalarParameter* ScalarParameter = Cast<UMaterialExpressionScalarParameter>(Expression);

				float DefaultValue;
				if (Properties->TryGetNumberField("DefaultValue", DefaultValue)) ScalarParameter->DefaultValue = DefaultValue;
				FString ExpressionGUID;
				if (Properties->TryGetStringField("ExpressionGUID", ExpressionGUID)) ScalarParameter->ExpressionGUID = FGuid(ExpressionGUID);
				FString ParameterName;
				if (Properties->TryGetStringField("ParameterName", ParameterName)) ScalarParameter->ParameterName = FName(ParameterName);

				Expression = ScalarParameter;
			} else if (Type->Type == "MaterialExpressionPanner") {
				UMaterialExpressionPanner* Panner = Cast<UMaterialExpressionPanner>(Expression);

				const TSharedPtr<FJsonObject>* CoordinatePtr = nullptr;
				if (Properties->TryGetObjectField("Coordinate", CoordinatePtr) && CoordinatePtr != nullptr) {
					FJsonObject* CoordinateObject = CoordinatePtr->Get();
					FName CoordinateExpressionName = GetExpressionName(CoordinateObject);
					if (CreatedExpressionMap.Contains(CoordinateExpressionName)) {
						FExpressionInput Coordinate = PopulateExpressionInput(CoordinateObject, *CreatedExpressionMap.Find(CoordinateExpressionName));
						Panner->Coordinate = Coordinate;
					}
				}

				const TSharedPtr<FJsonObject>* TimePtr = nullptr;
				if (Properties->TryGetObjectField("Time", TimePtr) && TimePtr != nullptr) {
					FJsonObject* TimeObject = TimePtr->Get();
					FName TimeExpressionName = GetExpressionName(TimeObject);
					if (CreatedExpressionMap.Contains(TimeExpressionName)) {
						FExpressionInput Time = PopulateExpressionInput(TimeObject, *CreatedExpressionMap.Find(TimeExpressionName));
						Panner->Time = Time;
					}
				}

				float SpeedX;
				if (Properties->TryGetNumberField("SpeedX", SpeedX)) Panner->SpeedX = SpeedX;
				float SpeedY;
				if (Properties->TryGetNumberField("SpeedY", SpeedY)) Panner->SpeedY = SpeedY;

				Expression = Panner;
			} else if (Type->Type == "MaterialExpressionNamedRerouteDeclaration") {
				UMaterialExpressionNamedRerouteDeclaration* NamedRerouteDeclaration = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						NamedRerouteDeclaration->Input = Input;
					}
				}

				FString VarName;
				if (Properties->TryGetStringField("Name", VarName)) NamedRerouteDeclaration->Name = FName(VarName);
				const TSharedPtr<FJsonObject>* NodeColor;
				if (Properties->TryGetObjectField("NodeColor", NodeColor)) NamedRerouteDeclaration->NodeColor = FMathUtilities::ObjectToLinearColor(NodeColor->Get());
				FString VariableGuid;
				if (Properties->TryGetStringField("VariableGuid", VariableGuid)) NamedRerouteDeclaration->VariableGuid = FGuid(VariableGuid);

				Expression = NamedRerouteDeclaration;
			} else if (Type->Type == "MaterialExpressionReroute") {
				UMaterialExpressionReroute* Reroute = Cast<UMaterialExpressionReroute>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Reroute->Input = Input;
					}
				}

				Expression = Reroute;
			} else if (Type->Type == "MaterialExpressionSubtract") {
				UMaterialExpressionSubtract* Subtract = Cast<UMaterialExpressionSubtract>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Subtract->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Subtract->B = B;
					}
				}

				Expression = Subtract;
			} else if (Type->Type == "MaterialExpressionSaturate") {
				UMaterialExpressionSaturate* Saturate = Cast<UMaterialExpressionSaturate>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Saturate->Input = Input;
					}
				}

				Expression = Saturate;
			} else if (Type->Type == "MaterialExpressionRotator") {
				UMaterialExpressionRotator* Rotator = static_cast<UMaterialExpressionRotator*>(Expression);

				const TSharedPtr<FJsonObject>* CoordinatePtr = nullptr;
				if (Properties->TryGetObjectField("Coordinate", CoordinatePtr) && CoordinatePtr != nullptr) {
					FJsonObject* CoordinateObject = CoordinatePtr->Get();
					FName CoordinateExpressionName = GetExpressionName(CoordinateObject);
					if (CreatedExpressionMap.Contains(CoordinateExpressionName)) {
						FExpressionInput Coordinate = PopulateExpressionInput(CoordinateObject, *CreatedExpressionMap.Find(CoordinateExpressionName));
						Rotator->Coordinate = Coordinate;
					}
				}

				const TSharedPtr<FJsonObject>* TimePtr = nullptr;
				if (Properties->TryGetObjectField("Time", TimePtr) && TimePtr != nullptr) {
					FJsonObject* TimeObject = TimePtr->Get();
					FName TimeExpressionName = GetExpressionName(TimeObject);
					if (CreatedExpressionMap.Contains(TimeExpressionName)) {
						FExpressionInput Time = PopulateExpressionInput(TimeObject, *CreatedExpressionMap.Find(TimeExpressionName));
						Rotator->Time = Time;
					}
				}

				Expression = Rotator;
			} else if (Type->Type == "MaterialExpressionMin") {
				UMaterialExpressionMin* Min = Cast<UMaterialExpressionMin>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Min->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Min->B = B;
					}
				}

				Expression = Min;
			} else if (Type->Type == "MaterialExpressionNamedRerouteUsage") {
				UMaterialExpressionNamedRerouteUsage* NamedRerouteUsage = Cast<UMaterialExpressionNamedRerouteUsage>(Expression);

				const TSharedPtr<FJsonObject>* DeclarationPtr = nullptr;
				if (Properties->TryGetObjectField("Declaration", DeclarationPtr) && DeclarationPtr != nullptr) {
					NamedRerouteUsage->Declaration = LoadObject<UMaterialExpressionNamedRerouteDeclaration>(DeclarationPtr);
				}

				FString DeclarationGuid;
				if (Properties->TryGetStringField("DeclarationGuid", DeclarationGuid)) NamedRerouteUsage->DeclarationGuid = FGuid(DeclarationGuid);

				Expression = NamedRerouteUsage;
			} else if (Type->Type == "MaterialExpressionSine") {
				UMaterialExpressionSine* Sine = Cast<UMaterialExpressionSine>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Sine->Input = Input;
					}
				}

				Expression = Sine;
			} else if (Type->Type == "MaterialExpressionSmoothStep") {
				UMaterialExpressionSmoothStep* SmoothStep = Cast<UMaterialExpressionSmoothStep>(Expression);

				const TSharedPtr<FJsonObject>* MinPtr = nullptr;
				if ((Properties->TryGetObjectField("min", MinPtr) || Properties->TryGetObjectField("Min", MinPtr)) && MinPtr != nullptr) {
					FJsonObject* MinObject = MinPtr->Get();
					FName MinExpressionName = GetExpressionName(MinObject);
					if (CreatedExpressionMap.Contains(MinExpressionName)) {
						FExpressionInput Min = PopulateExpressionInput(MinObject, *CreatedExpressionMap.Find(MinExpressionName));
						SmoothStep->Min = Min;
					}
				}

				const TSharedPtr<FJsonObject>* ValuePtr = nullptr;
				if (Properties->TryGetObjectField("Value", ValuePtr) && ValuePtr != nullptr) {
					FJsonObject* ValueObject = ValuePtr->Get();
					FName ValueExpressionName = GetExpressionName(ValueObject);
					if (CreatedExpressionMap.Contains(ValueExpressionName)) {
						FExpressionInput Value = PopulateExpressionInput(ValueObject, *CreatedExpressionMap.Find(ValueExpressionName));
						SmoothStep->Value = Value;
					}
				}

				float ConstMin;
				if (Properties->TryGetNumberField("ConstMin", ConstMin)) SmoothStep->ConstMin = ConstMin;
				float ConstMax;
				if (Properties->TryGetNumberField("ConstMax", ConstMax)) SmoothStep->ConstMax = ConstMax;

				Expression = SmoothStep;
			} else if (Type->Type == "MaterialExpressionAppendVector") {
				UMaterialExpressionAppendVector* AppendVector = Cast<UMaterialExpressionAppendVector>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						AppendVector->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						AppendVector->B = B;
					}
				}

				Expression = AppendVector;
			} else if (Type->Type == "MaterialExpressionDivide") {
				UMaterialExpressionDivide* Divide = Cast<UMaterialExpressionDivide>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Divide->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Divide->B = B;
					}
				}

				Expression = Divide;
			} else if (Type->Type == "MaterialExpressionDistance") {
				UMaterialExpressionDistance* Distance = Cast<UMaterialExpressionDistance>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						Distance->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						Distance->B = B;
					}
				}

				Expression = Distance;
			} else if (Type->Type == "MaterialExpressionStaticBool") {
				// No idea
			} else if (Type->Type == "MaterialExpressionStep") {
				UMaterialExpressionStep* Step = Cast<UMaterialExpressionStep>(Expression);

				const TSharedPtr<FJsonObject>* YPtr = nullptr;
				if (Properties->TryGetObjectField("Y", YPtr) && YPtr != nullptr) {
					FJsonObject* YObject = YPtr->Get();
					FName YExpressionName = GetExpressionName(YObject);
					if (CreatedExpressionMap.Contains(YExpressionName)) {
						FExpressionInput Y = PopulateExpressionInput(YObject, *CreatedExpressionMap.Find(YExpressionName));
						Step->Y = Y;
					}
				}

				const TSharedPtr<FJsonObject>* XPtr = nullptr;
				if (Properties->TryGetObjectField("X", XPtr) && XPtr != nullptr) {
					FJsonObject* XObject = XPtr->Get();
					FName XExpressionName = GetExpressionName(XObject);
					if (CreatedExpressionMap.Contains(XExpressionName)) {
						FExpressionInput X = PopulateExpressionInput(XObject, *CreatedExpressionMap.Find(XExpressionName));
						Step->X = X;
					}
				}

				Expression = Step;
			} else if (Type->Type == "MaterialExpressionDotProduct") {
				UMaterialExpressionDotProduct* DotProduct = Cast<UMaterialExpressionDotProduct>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						DotProduct->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						DotProduct->B = B;
					}
				}

				Expression = DotProduct;
			} else if (Type->Type == "MaterialExpressionStaticSwitch") {
				UMaterialExpressionStaticSwitch* StaticSwitch = Cast<UMaterialExpressionStaticSwitch>(Expression);

				const TSharedPtr<FJsonObject>* APtr = nullptr;
				if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
					FJsonObject* AObject = APtr->Get();
					FName AExpressionName = GetExpressionName(AObject);
					if (CreatedExpressionMap.Contains(AExpressionName)) {
						FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
						StaticSwitch->A = A;
					}
				}

				const TSharedPtr<FJsonObject>* BPtr = nullptr;
				if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
					FJsonObject* BObject = BPtr->Get();
					FName BExpressionName = GetExpressionName(BObject);
					if (CreatedExpressionMap.Contains(BExpressionName)) {
						FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
						StaticSwitch->B = B;
					}
				}

				const TSharedPtr<FJsonObject>* ValuePtr = nullptr;
				if (Properties->TryGetObjectField("Value", ValuePtr) && ValuePtr != nullptr) {
					FJsonObject* ValueObject = ValuePtr->Get();
					FName ValueExpressionName = GetExpressionName(ValueObject);
					if (CreatedExpressionMap.Contains(ValueExpressionName)) {
						FExpressionInput Value = PopulateExpressionInput(ValueObject, *CreatedExpressionMap.Find(ValueExpressionName));
						StaticSwitch->Value = Value;
					}
				}

				Expression = StaticSwitch;
			} else if (Type->Type == "MaterialExpressionPower") {
				UMaterialExpressionPower* Power = Cast<UMaterialExpressionPower>(Expression);

				const TSharedPtr<FJsonObject>* BasePtr = nullptr;
				if (Properties->TryGetObjectField("Base", BasePtr) && BasePtr != nullptr) {
					FJsonObject* BaseObject = BasePtr->Get();
					FName BaseExpressionName = GetExpressionName(BaseObject);
					if (CreatedExpressionMap.Contains(BaseExpressionName)) {
						FExpressionInput Base = PopulateExpressionInput(BaseObject, *CreatedExpressionMap.Find(BaseExpressionName));
						Power->Base = Base;
					}
				}

				Expression = Power;
			} else if (Type->Type == "MaterialExpressionRound") {
				UMaterialExpressionRound* Round = static_cast<UMaterialExpressionRound*>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Round->Input = Input;
					}
				}

				Expression = Round;
			} else if (Type->Type == "MaterialExpressionTextureSampleParameter2D") {
				UMaterialExpressionTextureSampleParameter2D* TextureSampleParameter2D = Cast<UMaterialExpressionTextureSampleParameter2D>(Expression);

				FString ExpressionGUID;
				if (Properties->TryGetStringField("ExpressionGUID", ExpressionGUID)) TextureSampleParameter2D->ExpressionGUID = FGuid(ExpressionGUID);
				FString ParameterName;
				if (Properties->TryGetStringField("ParameterName", ParameterName)) TextureSampleParameter2D->ParameterName = FName(ParameterName);

				const TSharedPtr<FJsonObject>* CoordinatesPtr = nullptr;
				if (Properties->TryGetObjectField("Coordinates", CoordinatesPtr) && CoordinatesPtr != nullptr) {
					FJsonObject* CoordinatesObject = CoordinatesPtr->Get();
					FName CoordinatesExpressionName = GetExpressionName(CoordinatesObject);
					if (CreatedExpressionMap.Contains(CoordinatesExpressionName)) {
						FExpressionInput Coordinates = PopulateExpressionInput(CoordinatesObject, *CreatedExpressionMap.Find(CoordinatesExpressionName));
						TextureSampleParameter2D->Coordinates = Coordinates;
					}
				}

				FString MipValueModeString;
				if (Properties->TryGetStringField("MipValueMode", MipValueModeString)) {
					ETextureMipValueMode MipValueMode;

					if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipLevel;
					else if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipBias;
					else if (MipValueModeString.EndsWith("TMVM_Derivative")) MipValueMode = TMVM_Derivative;
					else MipValueMode = TMVM_None;

					TextureSampleParameter2D->MipValueMode = MipValueMode;
				}

				const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
				if (Properties->TryGetObjectField("Texture", TexturePtr) && TexturePtr != nullptr) {
					TextureSampleParameter2D->Texture = LoadObject<UTexture>(TexturePtr);
				}

				Expression = TextureSampleParameter2D;
			} else if (Type->Type == "MaterialExpressionFloor") {
				UMaterialExpressionFloor* Floor = Cast<UMaterialExpressionFloor>(Expression);

				const TSharedPtr<FJsonObject>* InputPtr = nullptr;
				if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
					FJsonObject* InputObject = InputPtr->Get();
					FName InputExpressionName = GetExpressionName(InputObject);
					if (CreatedExpressionMap.Contains(InputExpressionName)) {
						FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
						Floor->Input = Input;
					}
				}

				Expression = Floor;
			} else if (Type->Type == "MaterialExpressionCustom") {
				UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression);

				FString Code;
				if (Properties->TryGetStringField("Code", Code)) Custom->Code = Code;

				FString OutputTypeString;
				if (Properties->TryGetStringField("OutputType", OutputTypeString)) {
					ECustomMaterialOutputType OutputType;

					if (OutputTypeString.EndsWith("CMOT_Float1")) OutputType = CMOT_Float1;
					else if (OutputTypeString.EndsWith("CMOT_Float2")) OutputType = CMOT_Float2;
					else if (OutputTypeString.EndsWith("CMOT_Float4")) OutputType = CMOT_Float4;
					else if (OutputTypeString.EndsWith("CMOT_MaterialAttributes")) OutputType = CMOT_MaterialAttributes;
					else OutputType = CMOT_Float3;

					Custom->OutputType = OutputType;
				}

				FString FuncDescription;
				if (Properties->TryGetStringField("Description", FuncDescription)) Custom->Description = FuncDescription;

				TArray<FCustomInput> Inputs;

				const TArray<TSharedPtr<FJsonValue>>* StringInputs;
				if (Properties->TryGetArrayField("Inputs", StringInputs)) {
					for (const TSharedPtr<FJsonValue> StringInput : *StringInputs) {
						if (StringInput->IsNull()) continue;

						FCustomInput Input;
						const TSharedPtr<FJsonObject>* InputPtr = nullptr;
						if (StringInput->AsObject()->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
							FJsonObject* InputObject = InputPtr->Get();
							FName InputExpressionName = GetExpressionName(InputObject);

							if (CreatedExpressionMap.Contains(InputExpressionName)) {
								FExpressionInput ExInput = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
								Input.Input = ExInput;
							}
						}

						FString InputName;
						if (StringInput->AsObject()->TryGetStringField("InputName", InputName)) Input.InputName = FName(InputName);

						Inputs.Add(Input);
					}
				}

				Custom->Inputs = Inputs;

				Expression = Custom;
			}

			AddGenerics(MaterialFunction, Expression, Properties);
			MaterialFunction->GetExpressionCollection().AddExpression(Expression);
		}

		TArray<FString> ExpressionCommentNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments;
		if (StringExpressionCollection->TryGetArrayField("EditorComments", StringExpressionComments)) {
			for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
				if (ExpressionComment->IsNull()) continue;
				FName ExportName = GetExportNameOfSubobject(ExpressionComment.Get()->AsObject()->GetStringField("ObjectName"));

				const TSharedPtr<FJsonObject> Comment = Exports.Find(ExportName)->Json->GetObjectField("Properties");
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
	FString Desc;
	bool bCommentBubbleVisible;
	bool bCollapsed;
	bool bRealtimePreview;

	if (Json->TryGetNumberField("MaterialExpressionEditorX", MaterialExpressionEditorX)) Expression->MaterialExpressionEditorX = MaterialExpressionEditorX;
	if (Json->TryGetNumberField("MaterialExpressionEditorY", MaterialExpressionEditorY)) Expression->MaterialExpressionEditorY = MaterialExpressionEditorY;
	Expression->MaterialExpressionGuid = FGuid(Json->GetStringField("MaterialExpressionGuid"));
	Expression->Function = MaterialFunction;
	if (Json->TryGetStringField("Desc", Desc)) Expression->Desc = Desc;
	if (Json->TryGetBoolField("bCommentBubbleVisible", bCommentBubbleVisible)) Expression->bCommentBubbleVisible = bCommentBubbleVisible;
	if (Json->TryGetBoolField("bCollapsed", bCollapsed)) Expression->bCollapsed = bCollapsed;
	if (Json->TryGetBoolField("bRealtimePreview", bRealtimePreview)) Expression->bRealtimePreview = bRealtimePreview;
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
	else if (Type == "MaterialExpressionOneMinus") Expression = NewObject<UMaterialExpressionOneMinus>(Parent, UMaterialExpressionOneMinus::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionMultiply") Expression = NewObject<UMaterialExpressionMultiply>(Parent, UMaterialExpressionMultiply::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionVectorParameter") Expression = NewObject<UMaterialExpressionVectorParameter>(Parent, UMaterialExpressionVectorParameter::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionTextureSample") Expression = NewObject<UMaterialExpressionTextureSample>(Parent, UMaterialExpressionTextureSample::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionMaterialFunctionCall") Expression = NewObject<UMaterialExpressionMaterialFunctionCall>(Parent, UMaterialExpressionMaterialFunctionCall::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionTextureObject") Expression = NewObject<UMaterialExpressionTextureObject>(Parent, UMaterialExpressionTextureObject::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionMax") Expression = NewObject<UMaterialExpressionMax>(Parent, UMaterialExpressionMax::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionTextureCoordinate") Expression = NewObject<UMaterialExpressionTextureCoordinate>(Parent, UMaterialExpressionTextureCoordinate::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionTime") Expression = NewObject<UMaterialExpressionTime>(Parent, UMaterialExpressionTime::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionScalarParameter") Expression = NewObject<UMaterialExpressionScalarParameter>(Parent, UMaterialExpressionScalarParameter::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionPanner") Expression = NewObject<UMaterialExpressionPanner>(Parent, UMaterialExpressionPanner::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionNamedRerouteDeclaration") Expression = NewObject<UMaterialExpressionNamedRerouteDeclaration>(Parent, UMaterialExpressionNamedRerouteDeclaration::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionReroute") Expression = NewObject<UMaterialExpressionReroute>(Parent, UMaterialExpressionReroute::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionSubtract") Expression = NewObject<UMaterialExpressionSubtract>(Parent, UMaterialExpressionSubtract::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionSaturate") Expression = NewObject<UMaterialExpressionSaturate>(Parent, UMaterialExpressionSaturate::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionRotator") Expression = static_cast<UMaterialExpressionRotator*>(NewObject<UMaterialExpression>(Parent, FindObject<UClass>(nullptr, TEXT("/Script/Engine.MaterialExpressionRotator")), Name, RF_Transactional));
	else if (Type == "MaterialExpressionMin") Expression = NewObject<UMaterialExpressionMin>(Parent, UMaterialExpressionMin::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionNamedRerouteUsage") Expression = NewObject<UMaterialExpressionNamedRerouteUsage>(Parent, UMaterialExpressionNamedRerouteUsage::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionSine") Expression = NewObject<UMaterialExpressionSine>(Parent, UMaterialExpressionSine::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionSmoothStep") Expression = NewObject<UMaterialExpressionSmoothStep>(Parent, UMaterialExpressionSmoothStep::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionAppendVector") Expression = NewObject<UMaterialExpressionAppendVector>(Parent, UMaterialExpressionAppendVector::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionDivide") Expression = NewObject<UMaterialExpressionDivide>(Parent, UMaterialExpressionDivide::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionDistance") Expression = NewObject<UMaterialExpressionDistance>(Parent, UMaterialExpressionDistance::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionStaticBool") Expression = NewObject<UMaterialExpressionStaticBool>(Parent, UMaterialExpressionStaticBool::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionStep") Expression = NewObject<UMaterialExpressionStep>(Parent, UMaterialExpressionStep::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionDotProduct") Expression = NewObject<UMaterialExpressionDotProduct>(Parent, UMaterialExpressionDotProduct::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionStaticSwitch") Expression = NewObject<UMaterialExpressionStaticSwitch>(Parent, UMaterialExpressionStaticSwitch::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionPower") Expression = NewObject<UMaterialExpressionPower>(Parent, UMaterialExpressionPower::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionRound") Expression = static_cast<UMaterialExpressionRound*>(NewObject<UMaterialExpression>(Parent, FindObject<UClass>(nullptr, TEXT("/Script/Engine.MaterialExpressionRound")), Name, RF_Transactional));
	else if (Type == "MaterialExpressionTextureSampleParameter2D") Expression = NewObject<UMaterialExpressionTextureSampleParameter2D>(Parent, UMaterialExpressionTextureSampleParameter2D::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionFloor") Expression = NewObject<UMaterialExpressionFloor>(Parent, UMaterialExpressionFloor::StaticClass(), Name, RF_Transactional);
	else if (Type == "MaterialExpressionCustom") Expression = NewObject<UMaterialExpressionCustom>(Parent, UMaterialExpressionCustom::StaticClass(), Name, RF_Transactional);

	if (!Expression) {
		UE_LOG(LogJson, Warning, TEXT("Missing support for expression type: \"%s\""), *Type.ToString())
		return nullptr;
	}

	return Expression;
}

FExpressionInput UMaterialFunctionImporter::PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression) {
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

	return Input;
}

FExpressionOutput UMaterialFunctionImporter::PopulateExpressionOutput(const FJsonObject* JsonProperties) {
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

FName UMaterialFunctionImporter::GetExpressionName(const FJsonObject* JsonProperties) {
	const TSharedPtr<FJsonValue> ExpressionField = JsonProperties->TryGetField("Expression");

	if (ExpressionField->IsNull()) return NAME_None;
	const TSharedPtr<FJsonObject> ExpressionObject = ExpressionField->AsObject();
	FString ObjectName;
	if (ExpressionObject->TryGetStringField("ObjectName", ObjectName)) {
		return GetExportNameOfSubobject(ObjectName);
	}

	return NAME_None;
}

FFunctionExpressionOutput UMaterialFunctionImporter::PopulateFuncExpressionOutput(const TSharedPtr<FJsonObject>& JsonProperties) {
	FFunctionExpressionOutput Output;

	FString ExpressionOutputId;
	if (JsonProperties->TryGetStringField("ExpressionOutputId", ExpressionOutputId)) Output.ExpressionOutputId = FGuid(ExpressionOutputId);

	const TSharedPtr<FJsonObject>* OutputPtr = nullptr;
	if (JsonProperties->TryGetObjectField("Output", OutputPtr) && OutputPtr != nullptr) {
		Output.Output = PopulateExpressionOutput(OutputPtr->Get());
	}

	return Output;
}

FFunctionExpressionInput UMaterialFunctionImporter::PopulateFuncExpressionInput(const TSharedPtr<FJsonObject>& JsonProperties, TMap<FName, UMaterialExpression*>& CreatedExpressionMap) {
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
