// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"

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
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionReroute.h"
#include "Materials/MaterialExpressionRotator.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureSample.h"
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
		MaterialFunction->Description = "Simple flow map function";
		MaterialFunction->bExposeToLibrary = true;
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
				FString Description;
				if (Properties->TryGetStringField("Description", Description)) FunctionInput->Description = Description;
				FunctionInput->Id = FGuid(Properties->GetStringField("ID"));

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

				const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
				if (Properties->TryGetObjectField("Texture", TexturePtr) && TexturePtr != nullptr) {
					TextureSample->Texture = LoadObject<UTexture>(TexturePtr);
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
			}

			AddGenerics(MaterialFunction, Expression, Properties);
			MaterialFunction->GetExpressionCollection().AddExpression(Expression);
		}

		TArray<FString> ExpressionCommentNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments;
		if (StringExpressionCollection->TryGetArrayField("EditorComments", StringExpressionComments)) {
			for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
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
