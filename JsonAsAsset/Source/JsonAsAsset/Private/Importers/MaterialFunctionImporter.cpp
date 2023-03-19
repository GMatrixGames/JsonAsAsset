// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"

#include "Curves/CurveLinearColorAtlas.h"
#include "Dom/JsonObject.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCeil.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstantBiasScale.h"
#include "Materials/MaterialExpressionCosine.h"
#include "Materials/MaterialExpressionCurveAtlasRowParameter.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDesaturation.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionFmod.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionIf.h"
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
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionStep.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureProperty.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTransformPosition.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Utilities/MathUtilities.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialExpressionFeatureLevelSwitch.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionWorldPosition.h"

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
		TMap<FName, FImportData> Exports;
		TArray<FName> ExpressionNames;
		const TSharedPtr<FJsonObject> EdProps = FindEditorOnlyData(JsonObject->GetStringField("Type"), MaterialFunction->GetName(), Exports, ExpressionNames)->GetObjectField("Properties");
		const TSharedPtr<FJsonObject> StringExpressionCollection = EdProps->GetObjectField("ExpressionCollection");

		// Map out each expression for easier access
		TMap<FName, UMaterialExpression*> CreatedExpressionMap = CreateExpressions(MaterialFunction, MaterialFunction->GetName(), ExpressionNames, Exports);

		// Iterate through all the expression names
		AddExpressions(MaterialFunction, ExpressionNames, Exports, CreatedExpressionMap);

		AddComments(MaterialFunction, StringExpressionCollection, Exports);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception))
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> UMaterialFunctionImporter::FindEditorOnlyData(const FString& Type, const FString& Outer, TMap<FName, FImportData>& OutExports, TArray<FName>& ExpressionNames) {
	TSharedPtr<FJsonObject> EditorOnlyData;

	for (const TSharedPtr<FJsonValue> Value : FilterExportsByOuter(Outer)) {
		TSharedPtr<FJsonObject> Object = TSharedPtr(Value->AsObject());

		FString ExType = Object->GetStringField("Type");
		FString Name = Object->GetStringField("Name");

		if (ExType == Type + "EditorOnlyData") {
			EditorOnlyData = Object;
			continue;
		}

		ExpressionNames.Add(FName(Name));
		OutExports.Add(FName(Name), FImportData(ExType, Outer, Object));
	}

	return EditorOnlyData;
}

TMap<FName, UMaterialExpression*> UMaterialFunctionImporter::CreateExpressions(UObject* Parent, const FString& Outer, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports) {
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
		if (Ex == nullptr)
			continue;

		CreatedExpressionMap.Add(Name, Ex);
	}

	return CreatedExpressionMap;
}

void UMaterialFunctionImporter::AddExpressions(UObject* Parent, TArray<FName>& ExpressionNames, TMap<FName, FImportData>& Exports, TMap<FName, UMaterialExpression*>& CreatedExpressionMap, bool bCheckOuter) {
	for (FName Name : ExpressionNames) {
		FImportData* Type = Exports.Find(Name);

		TSharedPtr<FJsonObject> Properties = Type->Json->GetObjectField("Properties");

		// Find the expression from FName
		if (!CreatedExpressionMap.Contains(Name)) continue;
		UMaterialExpression* Expression = *CreatedExpressionMap.Find(Name);

		if (bCheckOuter) {
			FString Outer;
			if (Type->Json->TryGetStringField("Outer", Outer) && Outer != Parent->GetName()) {
				continue;
			}
		}

		// UE_LOG(LogJson, Warning, TEXT("Export Name: %s"), *Name.ToString())

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
				EFunctionInputType InputType = FunctionInput->InputType;

				if (InputTypeString.EndsWith("FunctionInput_Scalar")) InputType = FunctionInput_Scalar;
				else if (InputTypeString.EndsWith("FunctionInput_Vector2")) InputType = FunctionInput_Vector2;
				else if (InputTypeString.EndsWith("FunctionInput_Vector3")) InputType = FunctionInput_Vector3;
				else if (InputTypeString.EndsWith("FunctionInput_Vector4")) InputType = FunctionInput_Vector4;
				else if (InputTypeString.EndsWith("FunctionInput_Texture2D")) InputType = FunctionInput_Texture2D;
				else if (InputTypeString.EndsWith("FunctionInput_TextureCube")) InputType = FunctionInput_TextureCube;
				else if (InputTypeString.EndsWith("FunctionInput_Texture2DArray")) InputType = FunctionInput_Texture2DArray;
				else if (InputTypeString.EndsWith("FunctionInput_VolumeTexture")) InputType = FunctionInput_VolumeTexture;
				else if (InputTypeString.EndsWith("FunctionInput_StaticBool")) InputType = FunctionInput_StaticBool;
				else if (InputTypeString.EndsWith("FunctionInput_MaterialAttributes")) InputType = FunctionInput_MaterialAttributes;
				else if (InputTypeString.EndsWith("FunctionInput_TextureExternal")) InputType = FunctionInput_TextureExternal;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				else if (InputTypeString.EndsWith("FunctionInput_Bool")) InputType = FunctionInput_Bool;
				else if (InputTypeString.EndsWith("FunctionInput_Substrate")) InputType = FunctionInput_Substrate;
#endif

				FunctionInput->InputType = InputType;
			}

			const TSharedPtr<FJsonObject>* PreviewValue;
			if (Properties->TryGetObjectField("PreviewValue", PreviewValue)) FunctionInput->PreviewValue = FMathUtilities::ObjectToVector4f(PreviewValue->Get());
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
			bool A;
			if (Properties->TryGetBoolField("A", A)) ComponentMask->A = A;

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
			bool bUseCustomPrimitiveData;
			if (Properties->TryGetBoolField("bUseCustomPrimitiveData", bUseCustomPrimitiveData)) VectorParameter->bUseCustomPrimitiveData = bUseCustomPrimitiveData;
			uint8 PrimitiveDataIndex;
			if (Properties->TryGetNumberField("PrimitiveDataIndex", PrimitiveDataIndex)) VectorParameter->PrimitiveDataIndex = PrimitiveDataIndex;

			const TSharedPtr<FJsonObject>* ChannelNames;
			if (Properties->TryGetObjectField("ChannelNames", ChannelNames)) {
				const TSharedPtr<FJsonObject>* R;
				if (ChannelNames->Get()->TryGetObjectField("R", R)) {
					VectorParameter->ChannelNames.R = FText::FromString(R->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* G;
				if (ChannelNames->Get()->TryGetObjectField("G", G)) {
					VectorParameter->ChannelNames.G = FText::FromString(G->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* B;
				if (ChannelNames->Get()->TryGetObjectField("B", B)) {
					VectorParameter->ChannelNames.B = FText::FromString(B->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* A;
				if (ChannelNames->Get()->TryGetObjectField("A", A)) {
					VectorParameter->ChannelNames.A = FText::FromString(A->Get()->GetStringField("SourceString"));
				}
			}

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

			const TSharedPtr<FJsonObject>* MipValuePtr = nullptr;
			if (Properties->TryGetObjectField("MipValue", MipValuePtr) && MipValuePtr != nullptr) {
				FJsonObject* MipValueObject = MipValuePtr->Get();
				FName MipValueExpressionName = GetExpressionName(MipValueObject);
				if (CreatedExpressionMap.Contains(MipValueExpressionName)) {
					FExpressionInput MipValue = PopulateExpressionInput(MipValueObject, *CreatedExpressionMap.Find(MipValueExpressionName));
					TextureSample->MipValue = MipValue;
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesDXPtr = nullptr;
			if (Properties->TryGetObjectField("CoordinatesDX", CoordinatesDXPtr) && CoordinatesDXPtr != nullptr) {
				FJsonObject* CoordinatesDXObject = CoordinatesDXPtr->Get();
				FName CoordinatesDXExpressionName = GetExpressionName(CoordinatesDXObject);
				if (CreatedExpressionMap.Contains(CoordinatesDXExpressionName)) {
					FExpressionInput CoordinatesDX = PopulateExpressionInput(CoordinatesDXObject, *CreatedExpressionMap.Find(CoordinatesDXExpressionName));
					TextureSample->CoordinatesDX = CoordinatesDX;
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesDYPtr = nullptr;
			if (Properties->TryGetObjectField("CoordinatesDY", CoordinatesDYPtr) && CoordinatesDYPtr != nullptr) {
				FJsonObject* CoordinatesDYObject = CoordinatesDYPtr->Get();
				FName CoordinatesDYExpressionName = GetExpressionName(CoordinatesDYObject);
				if (CreatedExpressionMap.Contains(CoordinatesDYExpressionName)) {
					FExpressionInput CoordinatesDY = PopulateExpressionInput(CoordinatesDYObject, *CreatedExpressionMap.Find(CoordinatesDYExpressionName));
					TextureSample->CoordinatesDY = CoordinatesDY;
				}
			}

			const TSharedPtr<FJsonObject>* AutomaticViewMipBiasValuePtr = nullptr;
			if (Properties->TryGetObjectField("AutomaticViewMipBiasValue", AutomaticViewMipBiasValuePtr) && AutomaticViewMipBiasValuePtr != nullptr) {
				FJsonObject* AutomaticViewMipBiasValueObject = AutomaticViewMipBiasValuePtr->Get();
				FName AutomaticViewMipBiasValueExpressionName = GetExpressionName(AutomaticViewMipBiasValueObject);
				if (CreatedExpressionMap.Contains(AutomaticViewMipBiasValueExpressionName)) {
					FExpressionInput AutomaticViewMipBiasValue = PopulateExpressionInput(AutomaticViewMipBiasValueObject, *CreatedExpressionMap.Find(AutomaticViewMipBiasValueExpressionName));
					TextureSample->AutomaticViewMipBiasValue = AutomaticViewMipBiasValue;
				}
			}

			FString MipValueModeString;
			if (Properties->TryGetStringField("MipValueMode", MipValueModeString)) {
				ETextureMipValueMode MipValueMode = TextureSample->MipValueMode;

				if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipLevel;
				else if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipBias;
				else if (MipValueModeString.EndsWith("TMVM_Derivative")) MipValueMode = TMVM_Derivative;

				TextureSample->MipValueMode = MipValueMode;
			}

			FString SampleSourceString;
			if (Properties->TryGetStringField("SamplerSource", SampleSourceString)) {
				ESamplerSourceMode SamplerSource = TextureSample->SamplerSource;

				if (SampleSourceString.EndsWith("SSM_Wrap_WorldGroupSettings")) SamplerSource = SSM_Wrap_WorldGroupSettings;
				else if (SampleSourceString.EndsWith("SSM_Clamp_WorldGroupSettings")) SamplerSource = SSM_Clamp_WorldGroupSettings;

				TextureSample->SamplerSource = SamplerSource;
			}

			bool AutomaticViewMipBias;
			if (Properties->TryGetBoolField("AutomaticViewMipBias", AutomaticViewMipBias)) TextureSample->AutomaticViewMipBias = AutomaticViewMipBias;

			Expression = TextureSample;
		} else if (Type->Type == "MaterialExpressionMaterialFunctionCall") {
			UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);

			const TSharedPtr<FJsonObject>* MaterialFunctionPtr = nullptr;
			if (Properties->TryGetObjectField("MaterialFunction", MaterialFunctionPtr) && MaterialFunctionPtr != nullptr) {
				MaterialFunctionCall->MaterialFunction = LoadObject<UMaterialFunction>(MaterialFunctionPtr);
			}

			const TArray<TSharedPtr<FJsonValue>>* FunctionInputsPtr;
			if (Properties->TryGetArrayField("FunctionInputs", FunctionInputsPtr)) {
				TArray<FFunctionExpressionInput> FunctionInputs;
				for (const TSharedPtr<FJsonValue> FunctionInputValue : *FunctionInputsPtr) {
					TSharedPtr<FJsonObject> FunctionInputObject = FunctionInputValue->AsObject();

					FunctionInputs.Add(PopulateFuncExpressionInput(FunctionInputObject, CreatedExpressionMap));
				}

				MaterialFunctionCall->FunctionInputs = FunctionInputs;
			}

			const TArray<TSharedPtr<FJsonValue>>* FunctionOutputsPtr;
			if (Properties->TryGetArrayField("FunctionOutputs", FunctionOutputsPtr)) {
				TArray<FFunctionExpressionOutput> FunctionOutputs;
				for (const TSharedPtr<FJsonValue> FunctionOutputValue : *FunctionOutputsPtr) {
					TSharedPtr<FJsonObject> FunctionOutputObject = FunctionOutputValue->AsObject();
					FunctionOutputs.Add(PopulateFuncExpressionOutput(FunctionOutputObject));
				}

				MaterialFunctionCall->FunctionOutputs = FunctionOutputs;
			}

			Expression = MaterialFunctionCall;
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

			float ConstA;
			if (Properties->TryGetNumberField("ConstA", ConstA)) Max->ConstA = ConstA;
			float ConstB;
			if (Properties->TryGetNumberField("ConstB", ConstB)) Max->ConstB = ConstB;

			Expression = Max;
		} else if (Type->Type == "MaterialExpressionTextureCoordinate") {
			UMaterialExpressionTextureCoordinate* TextureCoordinate = Cast<UMaterialExpressionTextureCoordinate>(Expression);

			int CoordinateIndex;
			if (Properties->TryGetNumberField("CoordinateIndex", CoordinateIndex)) TextureCoordinate->CoordinateIndex = CoordinateIndex;
			float UTiling;
			if (Properties->TryGetNumberField("UTiling", UTiling)) TextureCoordinate->UTiling = UTiling;
			float VTiling;
			if (Properties->TryGetNumberField("VTiling", VTiling)) TextureCoordinate->VTiling = VTiling;

			bool UnMirrorU;
			if (Properties->TryGetBoolField("UnMirrorU", UnMirrorU)) TextureCoordinate->UnMirrorU = UnMirrorU;
			bool UnMirrorV;
			if (Properties->TryGetBoolField("UnMirrorV", UnMirrorV)) TextureCoordinate->UnMirrorV = UnMirrorV;

			Expression = TextureCoordinate;
		} else if (Type->Type == "MaterialExpressionTime") {
			UMaterialExpressionTime* Time = Cast<UMaterialExpressionTime>(Expression);

			bool bIgnorePause;
			if (Properties->TryGetBoolField("bIgnorePause", bIgnorePause)) Time->bIgnorePause = bIgnorePause;
			bool bOverride_Period;
			if (Properties->TryGetBoolField("bOverride_Period", bOverride_Period)) Time->bOverride_Period = bOverride_Period;

			float Period;
			if (Properties->TryGetNumberField("Period", Period)) Time->Period = Period;

			Expression = Time;
		} else if (Type->Type == "MaterialExpressionScalarParameter") {
			UMaterialExpressionScalarParameter* ScalarParameter = Cast<UMaterialExpressionScalarParameter>(Expression);

			float DefaultValue;
			if (Properties->TryGetNumberField("DefaultValue", DefaultValue)) ScalarParameter->DefaultValue = DefaultValue;

			bool bUseCustomPrimitiveData;
			if (Properties->TryGetBoolField("bUseCustomPrimitiveData", bUseCustomPrimitiveData)) ScalarParameter->bUseCustomPrimitiveData = bUseCustomPrimitiveData;
			uint8 PrimitiveDataIndex;
			if (Properties->TryGetNumberField("PrimitiveDataIndex", PrimitiveDataIndex)) ScalarParameter->PrimitiveDataIndex = PrimitiveDataIndex;

			float SliderMin;
			if (Properties->TryGetNumberField("SliderMin", SliderMin)) ScalarParameter->SliderMin = SliderMin;
			float SliderMax;
			if (Properties->TryGetNumberField("SliderMax", SliderMax)) ScalarParameter->SliderMax = SliderMax;

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

			const TSharedPtr<FJsonObject>* SpeedPtr = nullptr;
			if (Properties->TryGetObjectField("Speed", SpeedPtr) && SpeedPtr != nullptr) {
				FJsonObject* SpeedObject = SpeedPtr->Get();
				FName SpeedExpressionName = GetExpressionName(SpeedObject);
				if (CreatedExpressionMap.Contains(SpeedExpressionName)) {
					FExpressionInput Speed = PopulateExpressionInput(SpeedObject, *CreatedExpressionMap.Find(SpeedExpressionName));
					Panner->Speed = Speed;
				}
			}

			float SpeedX;
			if (Properties->TryGetNumberField("SpeedX", SpeedX)) Panner->SpeedX = SpeedX;
			float SpeedY;
			if (Properties->TryGetNumberField("SpeedY", SpeedY)) Panner->SpeedY = SpeedY;
			int ConstCoordinate;
			if (Properties->TryGetNumberField("ConstCoordinate", ConstCoordinate)) Panner->ConstCoordinate = ConstCoordinate;

			bool bFractionalPart;
			if (Properties->TryGetBoolField("bFractionalPart", bFractionalPart)) Panner->bFractionalPart = bFractionalPart;

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

			float ConstA;
			if (Properties->TryGetNumberField("ConstA", ConstA)) Subtract->ConstA = ConstA;
			float ConstB;
			if (Properties->TryGetNumberField("ConstB", ConstB)) Subtract->ConstB = ConstB;

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

			float CenterX;
			if (Properties->TryGetNumberField("CenterX", CenterX)) Rotator->CenterX = CenterX;
			float CenterY;
			if (Properties->TryGetNumberField("CenterY", CenterY)) Rotator->CenterY = CenterY;
			float Speed;
			if (Properties->TryGetNumberField("Speed", Speed)) Rotator->Speed = Speed;

			int ConstCoordinate;
			if (Properties->TryGetNumberField("ConstCoordinate", ConstCoordinate)) Rotator->ConstCoordinate = ConstCoordinate;

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

			float ConstA;
			if (Properties->TryGetNumberField("ConstA", ConstA)) Min->ConstA = ConstA;
			float ConstB;
			if (Properties->TryGetNumberField("ConstB", ConstB)) Min->ConstB = ConstB;

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

			float Period;
			if (Properties->TryGetNumberField("Period", Period)) Sine->Period = Period;

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

			const TSharedPtr<FJsonObject>* MaxPtr = nullptr;
			if ((Properties->TryGetObjectField("max", MaxPtr) || Properties->TryGetObjectField("Max", MaxPtr)) && MaxPtr != nullptr) {
				FJsonObject* MaxObject = MaxPtr->Get();
				FName MaxExpressionName = GetExpressionName(MaxObject);
				if (CreatedExpressionMap.Contains(MaxExpressionName)) {
					FExpressionInput Max = PopulateExpressionInput(MaxObject, *CreatedExpressionMap.Find(MaxExpressionName));
					SmoothStep->Max = Max;
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
			float ConstValue;
			if (Properties->TryGetNumberField("ConstValue", ConstValue)) SmoothStep->ConstValue = ConstValue;

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

			float ConstA;
			if (Properties->TryGetNumberField("ConstA", ConstA)) Divide->ConstA = ConstA;
			float ConstB;
			if (Properties->TryGetNumberField("ConstB", ConstB)) Divide->ConstB = ConstB;

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
			UMaterialExpressionStaticBool* StaticBool = Cast<UMaterialExpressionStaticBool>(Expression);

			bool Value;
			if (Properties->TryGetBoolField("Value", Value)) StaticBool->Value = Value;

			Expression = StaticBool;
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

			float ConstY;
			if (Properties->TryGetNumberField("ConstY", ConstY)) Step->ConstY = ConstY;
			float ConstX;
			if (Properties->TryGetNumberField("ConstX", ConstX)) Step->ConstX = ConstX;

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

			bool DefaultValue;
			if (Properties->TryGetBoolField("DefaultValue", DefaultValue)) StaticSwitch->DefaultValue = DefaultValue;

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

			const TSharedPtr<FJsonObject>* ExponentPtr = nullptr;
			if (Properties->TryGetObjectField("Exponent", ExponentPtr) && ExponentPtr != nullptr) {
				FJsonObject* ExponentObject = ExponentPtr->Get();
				FName ExponentExpressionName = GetExpressionName(ExponentObject);
				if (CreatedExpressionMap.Contains(ExponentExpressionName)) {
					FExpressionInput Exponent = PopulateExpressionInput(ExponentObject, *CreatedExpressionMap.Find(ExponentExpressionName));
					Power->Exponent = Exponent;
				}
			}

			float ConstExponent;
			if (Properties->TryGetNumberField("ConstExponent", ConstExponent)) Power->ConstExponent = ConstExponent;

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
			FString Group;
			if (Properties->TryGetStringField("Group", Group)) TextureSampleParameter2D->Group = FName(Group);
			int SortPriority;
			if (Properties->TryGetNumberField("SortPriority", SortPriority)) TextureSampleParameter2D->SortPriority = SortPriority;

			const TSharedPtr<FJsonObject>* ChannelNames;
			if (Properties->TryGetObjectField("ChannelNames", ChannelNames)) {
				const TSharedPtr<FJsonObject>* R;
				if (ChannelNames->Get()->TryGetObjectField("R", R)) {
					TextureSampleParameter2D->ChannelNames.R = FText::FromString(R->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* G;
				if (ChannelNames->Get()->TryGetObjectField("G", G)) {
					TextureSampleParameter2D->ChannelNames.G = FText::FromString(G->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* B;
				if (ChannelNames->Get()->TryGetObjectField("B", B)) {
					TextureSampleParameter2D->ChannelNames.B = FText::FromString(B->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* A;
				if (ChannelNames->Get()->TryGetObjectField("A", A)) {
					TextureSampleParameter2D->ChannelNames.A = FText::FromString(A->Get()->GetStringField("SourceString"));
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesPtr = nullptr;
			if (Properties->TryGetObjectField("Coordinates", CoordinatesPtr) && CoordinatesPtr != nullptr) {
				FJsonObject* CoordinatesObject = CoordinatesPtr->Get();
				FName CoordinatesExpressionName = GetExpressionName(CoordinatesObject);
				if (CreatedExpressionMap.Contains(CoordinatesExpressionName)) {
					FExpressionInput Coordinates = PopulateExpressionInput(CoordinatesObject, *CreatedExpressionMap.Find(CoordinatesExpressionName));
					TextureSampleParameter2D->Coordinates = Coordinates;
				}
			}

			const TSharedPtr<FJsonObject>* TextureObjectPtr = nullptr;
			if (Properties->TryGetObjectField("TextureObject", TextureObjectPtr) && TextureObjectPtr != nullptr) {
				FJsonObject* TextureObjectObject = TextureObjectPtr->Get();
				FName TextureObjectExpressionName = GetExpressionName(TextureObjectObject);
				if (CreatedExpressionMap.Contains(TextureObjectExpressionName)) {
					FExpressionInput TextureObject = PopulateExpressionInput(TextureObjectObject, *CreatedExpressionMap.Find(TextureObjectExpressionName));
					TextureSampleParameter2D->TextureObject = TextureObject;
				}
			}

			const TSharedPtr<FJsonObject>* MipValuePtr = nullptr;
			if (Properties->TryGetObjectField("MipValue", MipValuePtr) && MipValuePtr != nullptr) {
				FJsonObject* MipValueObject = MipValuePtr->Get();
				FName MipValueExpressionName = GetExpressionName(MipValueObject);
				if (CreatedExpressionMap.Contains(MipValueExpressionName)) {
					FExpressionInput MipValue = PopulateExpressionInput(MipValueObject, *CreatedExpressionMap.Find(MipValueExpressionName));
					TextureSampleParameter2D->MipValue = MipValue;
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesDXPtr = nullptr;
			if (Properties->TryGetObjectField("CoordinatesDX", CoordinatesDXPtr) && CoordinatesDXPtr != nullptr) {
				FJsonObject* CoordinatesDXObject = CoordinatesDXPtr->Get();
				FName CoordinatesDXExpressionName = GetExpressionName(CoordinatesDXObject);
				if (CreatedExpressionMap.Contains(CoordinatesDXExpressionName)) {
					FExpressionInput CoordinatesDX = PopulateExpressionInput(CoordinatesDXObject, *CreatedExpressionMap.Find(CoordinatesDXExpressionName));
					TextureSampleParameter2D->CoordinatesDX = CoordinatesDX;
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesDYPtr = nullptr;
			if (Properties->TryGetObjectField("CoordinatesDY", CoordinatesDYPtr) && CoordinatesDYPtr != nullptr) {
				FJsonObject* CoordinatesDYObject = CoordinatesDYPtr->Get();
				FName CoordinatesDYExpressionName = GetExpressionName(CoordinatesDYObject);
				if (CreatedExpressionMap.Contains(CoordinatesDYExpressionName)) {
					FExpressionInput CoordinatesDY = PopulateExpressionInput(CoordinatesDYObject, *CreatedExpressionMap.Find(CoordinatesDYExpressionName));
					TextureSampleParameter2D->CoordinatesDY = CoordinatesDY;
				}
			}

			const TSharedPtr<FJsonObject>* AutomaticViewMipBiasValuePtr = nullptr;
			if (Properties->TryGetObjectField("AutomaticViewMipBiasValue", AutomaticViewMipBiasValuePtr) && AutomaticViewMipBiasValuePtr != nullptr) {
				FJsonObject* AutomaticViewMipBiasValueObject = AutomaticViewMipBiasValuePtr->Get();
				FName AutomaticViewMipBiasValueExpressionName = GetExpressionName(AutomaticViewMipBiasValueObject);
				if (CreatedExpressionMap.Contains(AutomaticViewMipBiasValueExpressionName)) {
					FExpressionInput AutomaticViewMipBiasValue = PopulateExpressionInput(AutomaticViewMipBiasValueObject, *CreatedExpressionMap.Find(AutomaticViewMipBiasValueExpressionName));
					TextureSampleParameter2D->AutomaticViewMipBiasValue = AutomaticViewMipBiasValue;
				}
			}

			FString MipValueModeString;
			if (Properties->TryGetStringField("MipValueMode", MipValueModeString)) {
				ETextureMipValueMode MipValueMode = TextureSampleParameter2D->MipValueMode;

				if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipLevel;
				else if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipBias;
				else if (MipValueModeString.EndsWith("TMVM_Derivative")) MipValueMode = TMVM_Derivative;

				TextureSampleParameter2D->MipValueMode = MipValueMode;
			}

			FString SampleSourceString;
			if (Properties->TryGetStringField("SamplerSource", SampleSourceString)) {
				ESamplerSourceMode SamplerSource = TextureSampleParameter2D->SamplerSource;

				if (SampleSourceString.EndsWith("SSM_Wrap_WorldGroupSettings")) SamplerSource = SSM_Wrap_WorldGroupSettings;
				else if (SampleSourceString.EndsWith("SSM_Clamp_WorldGroupSettings")) SamplerSource = SSM_Clamp_WorldGroupSettings;

				TextureSampleParameter2D->SamplerSource = SamplerSource;
			}

			bool AutomaticViewMipBias;
			if (Properties->TryGetBoolField("AutomaticViewMipBias", AutomaticViewMipBias)) TextureSampleParameter2D->AutomaticViewMipBias = AutomaticViewMipBias;

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
				ECustomMaterialOutputType OutputType = Custom->OutputType;

				if (OutputTypeString.EndsWith("CMOT_Float1")) OutputType = CMOT_Float1;
				else if (OutputTypeString.EndsWith("CMOT_Float2")) OutputType = CMOT_Float2;
				else if (OutputTypeString.EndsWith("CMOT_Float3")) OutputType = CMOT_Float3;
				else if (OutputTypeString.EndsWith("CMOT_Float4")) OutputType = CMOT_Float4;
				else if (OutputTypeString.EndsWith("CMOT_MaterialAttributes")) OutputType = CMOT_MaterialAttributes;

				Custom->OutputType = OutputType;
			}

			FString FuncDescription;
			if (Properties->TryGetStringField("Description", FuncDescription)) Custom->Description = FuncDescription;

			const TArray<TSharedPtr<FJsonValue>>* StringInputs;
			if (Properties->TryGetArrayField("Inputs", StringInputs)) {
				TArray<FCustomInput> Inputs;

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

				Custom->Inputs = Inputs;
			}

			const TArray<TSharedPtr<FJsonValue>>* StringAdditionalOutputs;
			if (Properties->TryGetArrayField("AdditionalOutputs", StringAdditionalOutputs)) {
				TArray<FCustomOutput> AdditionalOutputs;

				for (const TSharedPtr<FJsonValue> StringOutput : *StringAdditionalOutputs) {
					if (StringOutput->IsNull()) continue;

					FCustomOutput Output;

					FString OutputTypeString1;
					if (StringOutput->AsObject()->TryGetStringField("OutputType", OutputTypeString1)) {
						ECustomMaterialOutputType OutputType = CMOT_Float1;

						if (OutputTypeString1.EndsWith("CMOT_Float2")) OutputType = CMOT_Float2;
						else if (OutputTypeString1.EndsWith("CMOT_Float3")) OutputType = CMOT_Float3;
						else if (OutputTypeString1.EndsWith("CMOT_Float4")) OutputType = CMOT_Float4;
						else if (OutputTypeString1.EndsWith("CMOT_MaterialAttributes")) OutputType = CMOT_MaterialAttributes;

						Output.OutputType = OutputType;
					}

					FString OutputName;
					if (StringOutput->AsObject()->TryGetStringField("OutputName", OutputName)) Output.OutputName = FName(OutputName);

					AdditionalOutputs.Add(Output);
				}

				Custom->AdditionalOutputs = AdditionalOutputs;
			}

			const TArray<TSharedPtr<FJsonValue>>* StringAdditionalDefines;
			if (Properties->TryGetArrayField("AdditionalDefines", StringAdditionalDefines)) {
				TArray<FCustomDefine> AdditionalDefines;

				for (const TSharedPtr<FJsonValue> StringDefine : *StringAdditionalDefines) {
					if (StringDefine->IsNull()) continue;

					FCustomDefine Define;

					FString DefineName;
					if (StringDefine->AsObject()->TryGetStringField("DefineName", DefineName)) Define.DefineName = DefineName;
					FString DefineValue;
					if (StringDefine->AsObject()->TryGetStringField("DefineValue", DefineValue)) Define.DefineValue = DefineValue;

					AdditionalDefines.Add(Define);
				}

				Custom->AdditionalDefines = AdditionalDefines;
			}

			const TArray<TSharedPtr<FJsonValue>>* IncludeFilePathsPtr;
			if (Properties->TryGetArrayField("IncludeFilePaths", IncludeFilePathsPtr)) {
				Custom->IncludeFilePaths.Empty();

				for (const TSharedPtr<FJsonValue> IncludeFilePathsValue : *IncludeFilePathsPtr) {
					Custom->IncludeFilePaths.Add(IncludeFilePathsValue->AsString());
				}
			}

			Expression = Custom;
		} else if (Type->Type == "MaterialExpressionCeil") {
			UMaterialExpressionCeil* Ceil = Cast<UMaterialExpressionCeil>(Expression);

			const TSharedPtr<FJsonObject>* InputPtr = nullptr;
			if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
				FJsonObject* InputObject = InputPtr->Get();
				FName InputExpressionName = GetExpressionName(InputObject);
				if (CreatedExpressionMap.Contains(InputExpressionName)) {
					FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
					Ceil->Input = Input;
				}
			}

			Expression = Ceil;
		} else if (Type->Type == "MaterialExpressionIf") {
			UMaterialExpressionIf* If = Cast<UMaterialExpressionIf>(Expression);

			const TSharedPtr<FJsonObject>* APtr = nullptr;
			if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
				FJsonObject* AObject = APtr->Get();
				FName AExpressionName = GetExpressionName(AObject);
				if (CreatedExpressionMap.Contains(AExpressionName)) {
					FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
					If->A = A;
				}
			}

			const TSharedPtr<FJsonObject>* BPtr = nullptr;
			if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
				FJsonObject* BObject = BPtr->Get();
				FName BExpressionName = GetExpressionName(BObject);
				if (CreatedExpressionMap.Contains(BExpressionName)) {
					FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
					If->B = B;
				}
			}

			const TSharedPtr<FJsonObject>* AGreaterThanBPtr = nullptr;
			if (Properties->TryGetObjectField("AGreaterThanB", AGreaterThanBPtr) && AGreaterThanBPtr != nullptr) {
				FJsonObject* AGreaterThanBObject = AGreaterThanBPtr->Get();
				FName AGreaterThanBExpressionName = GetExpressionName(AGreaterThanBObject);
				if (CreatedExpressionMap.Contains(AGreaterThanBExpressionName)) {
					FExpressionInput AGreaterThanB = PopulateExpressionInput(AGreaterThanBObject, *CreatedExpressionMap.Find(AGreaterThanBExpressionName));
					If->AGreaterThanB = AGreaterThanB;
				}
			}

			const TSharedPtr<FJsonObject>* AEqualsBPtr = nullptr;
			if (Properties->TryGetObjectField("AEqualsB", AEqualsBPtr) && AEqualsBPtr != nullptr) {
				FJsonObject* AEqualsBObject = AEqualsBPtr->Get();
				FName AEqualsBExpressionName = GetExpressionName(AEqualsBObject);
				if (CreatedExpressionMap.Contains(AEqualsBExpressionName)) {
					FExpressionInput AEqualsB = PopulateExpressionInput(AEqualsBObject, *CreatedExpressionMap.Find(AEqualsBExpressionName));
					If->AEqualsB = AEqualsB;
				}
			}

			const TSharedPtr<FJsonObject>* ALessThanBPtr = nullptr;
			if (Properties->TryGetObjectField("ALessThanB", ALessThanBPtr) && ALessThanBPtr != nullptr) {
				FJsonObject* ALessThanBObject = ALessThanBPtr->Get();
				FName ALessThanBExpressionName = GetExpressionName(ALessThanBObject);
				if (CreatedExpressionMap.Contains(ALessThanBExpressionName)) {
					FExpressionInput ALessThanB = PopulateExpressionInput(ALessThanBObject, *CreatedExpressionMap.Find(ALessThanBExpressionName));
					If->ALessThanB = ALessThanB;
				}
			}

			float EqualsThreshold;
			if (Properties->TryGetNumberField("EqualsThreshold", EqualsThreshold)) If->EqualsThreshold = EqualsThreshold;
			float ConstB;
			if (Properties->TryGetNumberField("ConstB", ConstB)) If->ConstB = ConstB;

			Expression = If;
		} else if (Type->Type == "MaterialExpressionCosine") {
			UMaterialExpressionCosine* Cosine = Cast<UMaterialExpressionCosine>(Expression);

			const TSharedPtr<FJsonObject>* InputPtr = nullptr;
			if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
				FJsonObject* InputObject = InputPtr->Get();
				FName InputExpressionName = GetExpressionName(InputObject);
				if (CreatedExpressionMap.Contains(InputExpressionName)) {
					FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
					Cosine->Input = Input;
				}
			}

			float Period;
			if (Properties->TryGetNumberField("Period", Period)) Cosine->Period = Period;

			Expression = Cosine;
		} else if (Type->Type == "MaterialExpressionDesaturation") {
			UMaterialExpressionDesaturation* Desaturation = Cast<UMaterialExpressionDesaturation>(Expression);

			const TSharedPtr<FJsonObject>* InputPtr = nullptr;
			if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
				FJsonObject* InputObject = InputPtr->Get();
				FName InputExpressionName = GetExpressionName(InputObject);
				if (CreatedExpressionMap.Contains(InputExpressionName)) {
					FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
					Desaturation->Input = Input;
				}
			}

			const TSharedPtr<FJsonObject>* FractionPtr = nullptr;
			if (Properties->TryGetObjectField("Fraction", FractionPtr) && FractionPtr != nullptr) {
				FJsonObject* FractionObject = FractionPtr->Get();
				FName FractionExpressionName = GetExpressionName(FractionObject);
				if (CreatedExpressionMap.Contains(FractionExpressionName)) {
					FExpressionInput Fraction = PopulateExpressionInput(FractionObject, *CreatedExpressionMap.Find(FractionExpressionName));
					Desaturation->Fraction = Fraction;
				}
			}

			const TSharedPtr<FJsonObject>* LuminanceFactors;
			if (Properties->TryGetObjectField("LuminanceFactors", LuminanceFactors)) Desaturation->LuminanceFactors = FMathUtilities::ObjectToLinearColor(LuminanceFactors->Get());

			Expression = Desaturation;
		} else if (Type->Type == "MaterialExpressionClamp") {
			UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(Expression);

			const TSharedPtr<FJsonObject>* InputPtr = nullptr;
			if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
				FJsonObject* InputObject = InputPtr->Get();
				FName InputExpressionName = GetExpressionName(InputObject);
				if (CreatedExpressionMap.Contains(InputExpressionName)) {
					FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
					Clamp->Input = Input;
				}
			}

			const TSharedPtr<FJsonObject>* MinPtr = nullptr;
			if ((Properties->TryGetObjectField("min", MinPtr) || Properties->TryGetObjectField("Min", MinPtr)) && MinPtr != nullptr) {
				FJsonObject* MinObject = MinPtr->Get();
				FName MinExpressionName = GetExpressionName(MinObject);
				if (CreatedExpressionMap.Contains(MinExpressionName)) {
					FExpressionInput Min = PopulateExpressionInput(MinObject, *CreatedExpressionMap.Find(MinExpressionName));
					Clamp->Min = Min;
				}
			}

			const TSharedPtr<FJsonObject>* MaxPtr = nullptr;
			if ((Properties->TryGetObjectField("max", MaxPtr) || Properties->TryGetObjectField("Max", MaxPtr)) && MaxPtr != nullptr) {
				FJsonObject* MaxObject = MaxPtr->Get();
				FName MaxExpressionName = GetExpressionName(MaxObject);
				if (CreatedExpressionMap.Contains(MaxExpressionName)) {
					FExpressionInput Max = PopulateExpressionInput(MaxObject, *CreatedExpressionMap.Find(MaxExpressionName));
					Clamp->Max = Max;
				}
			}

			FString ClampModeString;
			if (Properties->TryGetStringField("ClampMode", ClampModeString)) {
				EClampMode ClampMode = Clamp->ClampMode;

				if (ClampModeString.EndsWith("CMODE_Clamp")) ClampMode = CMODE_Clamp;
				else if (ClampModeString.EndsWith("CMODE_ClampMin")) ClampMode = CMODE_ClampMin;
				else if (ClampModeString.EndsWith("CMODE_ClampMax")) ClampMode = CMODE_ClampMax;

				Clamp->ClampMode = ClampMode;
			}

			float MinDefault;
			if (Properties->TryGetNumberField("MinDefault", MinDefault)) Clamp->MinDefault = MinDefault;
			float MaxDefault;
			if (Properties->TryGetNumberField("MaxDefault", MaxDefault)) Clamp->MaxDefault = MaxDefault;

			Expression = Clamp;
		} else if (Type->Type == "MaterialExpressionTransformPosition") {
			UMaterialExpressionTransformPosition* TransformPosition = Cast<UMaterialExpressionTransformPosition>(Expression);

			const TSharedPtr<FJsonObject>* InputPtr = nullptr;
			if (Properties->TryGetObjectField("Input", InputPtr) && InputPtr != nullptr) {
				FJsonObject* InputObject = InputPtr->Get();
				FName InputExpressionName = GetExpressionName(InputObject);
				if (CreatedExpressionMap.Contains(InputExpressionName)) {
					FExpressionInput Input = PopulateExpressionInput(InputObject, *CreatedExpressionMap.Find(InputExpressionName));
					TransformPosition->Input = Input;
				}
			}

			FString TransformSourceTypeString;
			if (Properties->TryGetStringField("TransformSourceType", TransformSourceTypeString)) {
				EMaterialPositionTransformSource TransformSourceType = TransformPosition->TransformSourceType;

				if (TransformSourceTypeString.EndsWith("TRANSFORMPOSSOURCE_Local")) TransformSourceType = TRANSFORMPOSSOURCE_Local;
				else if (TransformSourceTypeString.EndsWith("TRANSFORMPOSSOURCE_World")) TransformSourceType = TRANSFORMPOSSOURCE_World;
				else if (TransformSourceTypeString.EndsWith("TRANSFORMPOSSOURCE_TranslatedWorld")) TransformSourceType = TRANSFORMPOSSOURCE_TranslatedWorld;
				else if (TransformSourceTypeString.EndsWith("TRANSFORMPOSSOURCE_View")) TransformSourceType = TRANSFORMPOSSOURCE_View;
				else if (TransformSourceTypeString.EndsWith("TRANSFORMPOSSOURCE_Instance")) TransformSourceType = TRANSFORMPOSSOURCE_Instance;

				TransformPosition->TransformSourceType = TransformSourceType;
			}

			FString TransformTypeString;
			if (Properties->TryGetStringField("TransformType", TransformTypeString)) {
				EMaterialPositionTransformSource TransformType = TransformPosition->TransformSourceType;

				if (TransformTypeString.EndsWith("TRANSFORMPOSSOURCE_Local")) TransformType = TRANSFORMPOSSOURCE_Local;
				else if (TransformTypeString.EndsWith("TRANSFORMPOSSOURCE_World")) TransformType = TRANSFORMPOSSOURCE_World;
				else if (TransformTypeString.EndsWith("TRANSFORMPOSSOURCE_TranslatedWorld")) TransformType = TRANSFORMPOSSOURCE_TranslatedWorld;
				else if (TransformTypeString.EndsWith("TRANSFORMPOSSOURCE_View")) TransformType = TRANSFORMPOSSOURCE_View;
				else if (TransformTypeString.EndsWith("TRANSFORMPOSSOURCE_Instance")) TransformType = TRANSFORMPOSSOURCE_Instance;

				TransformPosition->TransformType = TransformType;
			}

			Expression = TransformPosition;
		} else if (Type->Type == "MaterialExpressionSphereMask") {
			UMaterialExpressionSphereMask* SphereMask = Cast<UMaterialExpressionSphereMask>(Expression);

			const TSharedPtr<FJsonObject>* APtr = nullptr;
			if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
				FJsonObject* AObject = APtr->Get();
				FName AExpressionName = GetExpressionName(AObject);
				if (CreatedExpressionMap.Contains(AExpressionName)) {
					FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
					SphereMask->A = A;
				}
			}

			const TSharedPtr<FJsonObject>* BPtr = nullptr;
			if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
				FJsonObject* BObject = BPtr->Get();
				FName BExpressionName = GetExpressionName(BObject);
				if (CreatedExpressionMap.Contains(BExpressionName)) {
					FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
					SphereMask->B = B;
				}
			}

			const TSharedPtr<FJsonObject>* RadiusPtr = nullptr;
			if (Properties->TryGetObjectField("Radius", RadiusPtr) && RadiusPtr != nullptr) {
				FJsonObject* RadiusObject = RadiusPtr->Get();
				FName RadiusExpressionName = GetExpressionName(RadiusObject);
				if (CreatedExpressionMap.Contains(RadiusExpressionName)) {
					FExpressionInput Radius = PopulateExpressionInput(RadiusObject, *CreatedExpressionMap.Find(RadiusExpressionName));
					SphereMask->Radius = Radius;
				}
			}

			const TSharedPtr<FJsonObject>* HardnessPtr = nullptr;
			if (Properties->TryGetObjectField("Hardness", HardnessPtr) && HardnessPtr != nullptr) {
				FJsonObject* HardnessObject = HardnessPtr->Get();
				FName HardnessExpressionName = GetExpressionName(HardnessObject);
				if (CreatedExpressionMap.Contains(HardnessExpressionName)) {
					FExpressionInput Hardness = PopulateExpressionInput(HardnessObject, *CreatedExpressionMap.Find(HardnessExpressionName));
					SphereMask->Hardness = Hardness;
				}
			}

			float AttenuationRadius;
			if (Properties->TryGetNumberField("AttenuationRadius", AttenuationRadius)) SphereMask->AttenuationRadius = AttenuationRadius;
			float HardnessPercent;
			if (Properties->TryGetNumberField("HardnessPercent", HardnessPercent)) SphereMask->HardnessPercent = HardnessPercent;

			Expression = SphereMask;
		} else if (Type->Type == "MaterialExpressionCurveAtlasRowParameter") {
			UMaterialExpressionCurveAtlasRowParameter* CurveAtlasRowParameter = Cast<UMaterialExpressionCurveAtlasRowParameter>(Expression);

			const TSharedPtr<FJsonObject>* CurvePtr = nullptr;
			if (Properties->TryGetObjectField("Curve", CurvePtr) && CurvePtr != nullptr) {
				CurveAtlasRowParameter->Curve = LoadObject<UCurveLinearColor>(CurvePtr);
			}

			const TSharedPtr<FJsonObject>* AtlasPtr = nullptr;
			if (Properties->TryGetObjectField("Atlas", AtlasPtr) && AtlasPtr != nullptr) {
				CurveAtlasRowParameter->Atlas = LoadObject<UCurveLinearColorAtlas>(AtlasPtr);
			}

			const TSharedPtr<FJsonObject>* InputTimePtr = nullptr;
			if (Properties->TryGetObjectField("InputTime", InputTimePtr) && InputTimePtr != nullptr) {
				FJsonObject* InputTimeObject = InputTimePtr->Get();
				FName InputTimeExpressionName = GetExpressionName(InputTimeObject);
				if (CreatedExpressionMap.Contains(InputTimeExpressionName)) {
					FExpressionInput InputTime = PopulateExpressionInput(InputTimeObject, *CreatedExpressionMap.Find(InputTimeExpressionName));
					CurveAtlasRowParameter->InputTime = InputTime;
				}
			}

			Expression = CurveAtlasRowParameter;
		} else if (Type->Type == "MaterialExpressionTextureObjectParameter") {
			UMaterialExpressionTextureObjectParameter* TextureObjectParameter = Cast<UMaterialExpressionTextureObjectParameter>(Expression);

			FString ExpressionGUID;
			if (Properties->TryGetStringField("ExpressionGUID", ExpressionGUID)) TextureObjectParameter->ExpressionGUID = FGuid(ExpressionGUID);
			FString ParameterName;
			if (Properties->TryGetStringField("ParameterName", ParameterName)) TextureObjectParameter->ParameterName = FName(ParameterName);
			FString Group;
			if (Properties->TryGetStringField("Group", Group)) TextureObjectParameter->Group = FName(Group);
			int SortPriority;
			if (Properties->TryGetNumberField("SortPriority", SortPriority)) TextureObjectParameter->SortPriority = SortPriority;

			const TSharedPtr<FJsonObject>* ChannelNames;
			if (Properties->TryGetObjectField("ChannelNames", ChannelNames)) {
				const TSharedPtr<FJsonObject>* R;
				if (ChannelNames->Get()->TryGetObjectField("R", R)) {
					TextureObjectParameter->ChannelNames.R = FText::FromString(R->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* G;
				if (ChannelNames->Get()->TryGetObjectField("G", G)) {
					TextureObjectParameter->ChannelNames.G = FText::FromString(G->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* B;
				if (ChannelNames->Get()->TryGetObjectField("B", B)) {
					TextureObjectParameter->ChannelNames.B = FText::FromString(B->Get()->GetStringField("SourceString"));
				}
				const TSharedPtr<FJsonObject>* A;
				if (ChannelNames->Get()->TryGetObjectField("A", A)) {
					TextureObjectParameter->ChannelNames.A = FText::FromString(A->Get()->GetStringField("SourceString"));
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesPtr = nullptr;
			if (Properties->TryGetObjectField("Coordinates", CoordinatesPtr) && CoordinatesPtr != nullptr) {
				FJsonObject* CoordinatesObject = CoordinatesPtr->Get();
				FName CoordinatesExpressionName = GetExpressionName(CoordinatesObject);
				if (CreatedExpressionMap.Contains(CoordinatesExpressionName)) {
					FExpressionInput Coordinates = PopulateExpressionInput(CoordinatesObject, *CreatedExpressionMap.Find(CoordinatesExpressionName));
					TextureObjectParameter->Coordinates = Coordinates;
				}
			}

			const TSharedPtr<FJsonObject>* TextureObjectPtr = nullptr;
			if (Properties->TryGetObjectField("TextureObject", TextureObjectPtr) && TextureObjectPtr != nullptr) {
				FJsonObject* TextureObjectObject = TextureObjectPtr->Get();
				FName TextureObjectExpressionName = GetExpressionName(TextureObjectObject);
				if (CreatedExpressionMap.Contains(TextureObjectExpressionName)) {
					FExpressionInput TextureObject = PopulateExpressionInput(TextureObjectObject, *CreatedExpressionMap.Find(TextureObjectExpressionName));
					TextureObjectParameter->TextureObject = TextureObject;
				}
			}

			const TSharedPtr<FJsonObject>* MipValuePtr = nullptr;
			if (Properties->TryGetObjectField("MipValue", MipValuePtr) && MipValuePtr != nullptr) {
				FJsonObject* MipValueObject = MipValuePtr->Get();
				FName MipValueExpressionName = GetExpressionName(MipValueObject);
				if (CreatedExpressionMap.Contains(MipValueExpressionName)) {
					FExpressionInput MipValue = PopulateExpressionInput(MipValueObject, *CreatedExpressionMap.Find(MipValueExpressionName));
					TextureObjectParameter->MipValue = MipValue;
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesDXPtr = nullptr;
			if (Properties->TryGetObjectField("CoordinatesDX", CoordinatesDXPtr) && CoordinatesDXPtr != nullptr) {
				FJsonObject* CoordinatesDXObject = CoordinatesDXPtr->Get();
				FName CoordinatesDXExpressionName = GetExpressionName(CoordinatesDXObject);
				if (CreatedExpressionMap.Contains(CoordinatesDXExpressionName)) {
					FExpressionInput CoordinatesDX = PopulateExpressionInput(CoordinatesDXObject, *CreatedExpressionMap.Find(CoordinatesDXExpressionName));
					TextureObjectParameter->CoordinatesDX = CoordinatesDX;
				}
			}

			const TSharedPtr<FJsonObject>* CoordinatesDYPtr = nullptr;
			if (Properties->TryGetObjectField("CoordinatesDY", CoordinatesDYPtr) && CoordinatesDYPtr != nullptr) {
				FJsonObject* CoordinatesDYObject = CoordinatesDYPtr->Get();
				FName CoordinatesDYExpressionName = GetExpressionName(CoordinatesDYObject);
				if (CreatedExpressionMap.Contains(CoordinatesDYExpressionName)) {
					FExpressionInput CoordinatesDY = PopulateExpressionInput(CoordinatesDYObject, *CreatedExpressionMap.Find(CoordinatesDYExpressionName));
					TextureObjectParameter->CoordinatesDY = CoordinatesDY;
				}
			}

			const TSharedPtr<FJsonObject>* AutomaticViewMipBiasValuePtr = nullptr;
			if (Properties->TryGetObjectField("AutomaticViewMipBiasValue", AutomaticViewMipBiasValuePtr) && AutomaticViewMipBiasValuePtr != nullptr) {
				FJsonObject* AutomaticViewMipBiasValueObject = AutomaticViewMipBiasValuePtr->Get();
				FName AutomaticViewMipBiasValueExpressionName = GetExpressionName(AutomaticViewMipBiasValueObject);
				if (CreatedExpressionMap.Contains(AutomaticViewMipBiasValueExpressionName)) {
					FExpressionInput AutomaticViewMipBiasValue = PopulateExpressionInput(AutomaticViewMipBiasValueObject, *CreatedExpressionMap.Find(AutomaticViewMipBiasValueExpressionName));
					TextureObjectParameter->AutomaticViewMipBiasValue = AutomaticViewMipBiasValue;
				}
			}

			FString MipValueModeString;
			if (Properties->TryGetStringField("MipValueMode", MipValueModeString)) {
				ETextureMipValueMode MipValueMode = TextureObjectParameter->MipValueMode;

				if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipLevel;
				else if (MipValueModeString.EndsWith("TMVM_MipLevel")) MipValueMode = TMVM_MipBias;
				else if (MipValueModeString.EndsWith("TMVM_Derivative")) MipValueMode = TMVM_Derivative;

				TextureObjectParameter->MipValueMode = MipValueMode;
			}

			FString SampleSourceString;
			if (Properties->TryGetStringField("SamplerSource", SampleSourceString)) {
				ESamplerSourceMode SamplerSource = TextureObjectParameter->SamplerSource;

				if (SampleSourceString.EndsWith("SSM_FromTextureAsset")) SamplerSource = SSM_FromTextureAsset;
				else if (SampleSourceString.EndsWith("SSM_Wrap_WorldGroupSettings")) SamplerSource = SSM_Wrap_WorldGroupSettings;
				else if (SampleSourceString.EndsWith("SSM_Clamp_WorldGroupSettings")) SamplerSource = SSM_Clamp_WorldGroupSettings;

				TextureObjectParameter->SamplerSource = SamplerSource;
			}

			bool AutomaticViewMipBias;
			if (Properties->TryGetBoolField("AutomaticViewMipBias", AutomaticViewMipBias)) TextureObjectParameter->AutomaticViewMipBias = AutomaticViewMipBias;

			Expression = TextureObjectParameter;
		} else if (Type->Type == "MaterialExpressionFmod") {
			UMaterialExpressionFmod* Fmod = Cast<UMaterialExpressionFmod>(Expression);

			const TSharedPtr<FJsonObject>* APtr = nullptr;
			if (Properties->TryGetObjectField("A", APtr) && APtr != nullptr) {
				FJsonObject* AObject = APtr->Get();
				FName AExpressionName = GetExpressionName(AObject);
				if (CreatedExpressionMap.Contains(AExpressionName)) {
					FExpressionInput A = PopulateExpressionInput(AObject, *CreatedExpressionMap.Find(AExpressionName));
					Fmod->A = A;
				}
			}

			const TSharedPtr<FJsonObject>* BPtr = nullptr;
			if (Properties->TryGetObjectField("B", BPtr) && BPtr != nullptr) {
				FJsonObject* BObject = BPtr->Get();
				FName BExpressionName = GetExpressionName(BObject);
				if (CreatedExpressionMap.Contains(BExpressionName)) {
					FExpressionInput B = PopulateExpressionInput(BObject, *CreatedExpressionMap.Find(BExpressionName));
					Fmod->B = B;
				}
			}

			Expression = Fmod;
		} else if (Type->Type == "MaterialExpressionTextureProperty") {
			UMaterialExpressionTextureProperty* TextureProperty = Cast<UMaterialExpressionTextureProperty>(Expression);

			const TSharedPtr<FJsonObject>* TextureObjectPtr = nullptr;
			if (Properties->TryGetObjectField("TextureObject", TextureObjectPtr) && TextureObjectPtr != nullptr) {
				FJsonObject* TextureObjectObject = TextureObjectPtr->Get();
				FName TextureObjectExpressionName = GetExpressionName(TextureObjectObject);
				if (CreatedExpressionMap.Contains(TextureObjectExpressionName)) {
					FExpressionInput TextureObject = PopulateExpressionInput(TextureObjectObject, *CreatedExpressionMap.Find(TextureObjectExpressionName));
					TextureProperty->TextureObject = TextureObject;
				}
			}

			FString PropertyString;
			if (Properties->TryGetStringField("Property", PropertyString)) {
				EMaterialExposedTextureProperty Property = TextureProperty->Property;

				if (PropertyString.EndsWith("TMTM_TextureSize")) Property = TMTM_TextureSize;
				else if (PropertyString.EndsWith("TMTM_TexelSize")) Property = TMTM_TexelSize;

				TextureProperty->Property = Property;
			}

			Expression = TextureProperty;
		} else if (Type->Type == "MaterialExpressionWorldPosition") {
			UMaterialExpressionWorldPosition* WorldPosition = Cast<UMaterialExpressionWorldPosition>(Expression);

			FString WorldPositionShaderOffsetString;
			if (Properties->TryGetStringField("WorldPositionShaderOffset", WorldPositionShaderOffsetString)) {
				EWorldPositionIncludedOffsets WorldPositionShaderOffset = WorldPosition->WorldPositionShaderOffset;

				if (WorldPositionShaderOffsetString.EndsWith("WPT_Default")) WorldPositionShaderOffset = WPT_Default;
				else if (WorldPositionShaderOffsetString.EndsWith("WPT_ExcludeAllShaderOffsets")) WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
				else if (WorldPositionShaderOffsetString.EndsWith("WPT_CameraRelative")) WorldPositionShaderOffset = WPT_CameraRelative;
				else if (WorldPositionShaderOffsetString.EndsWith("WPT_CameraRelativeNoOffsets")) WorldPositionShaderOffset = WPT_CameraRelativeNoOffsets;

				WorldPosition->WorldPositionShaderOffset = WorldPositionShaderOffset;
			}

			Expression = WorldPosition;
		} else if (Type->Type == "MaterialExpressionNormalize") {
			UMaterialExpressionNormalize* Normalize = Cast<UMaterialExpressionNormalize>(Expression);

			const TSharedPtr<FJsonObject>* VectorInputPtr = nullptr;
			if (Properties->TryGetObjectField("VectorInput", VectorInputPtr) && VectorInputPtr != nullptr) {
				FJsonObject* VectorInputObject = VectorInputPtr->Get();
				FName VectorInputExpressionName = GetExpressionName(VectorInputObject);
				if (CreatedExpressionMap.Contains(VectorInputExpressionName)) {
					FExpressionInput VectorInput = PopulateExpressionInput(VectorInputObject, *CreatedExpressionMap.Find(VectorInputExpressionName));
					Normalize->VectorInput = VectorInput;
				}
			}

			Expression = Normalize;
		} else if (Type->Type == "MaterialExpressionDynamicParameter") {
			UMaterialExpressionDynamicParameter* DynamicParameter = Cast<UMaterialExpressionDynamicParameter>(Expression);

			const TArray<TSharedPtr<FJsonValue>>* ParamNamesPtr;
			if (Properties->TryGetArrayField("ParamNames", ParamNamesPtr)) {
				DynamicParameter->ParamNames.Empty();

				for (const TSharedPtr<FJsonValue> ReroutePinsValue : *ParamNamesPtr) {
					DynamicParameter->ParamNames.Add(ReroutePinsValue->AsString());
				}
			}

			const TSharedPtr<FJsonObject>* DefaultValue;
			if (Properties->TryGetObjectField("DefaultValue", DefaultValue)) DynamicParameter->DefaultValue = FMathUtilities::ObjectToLinearColor(DefaultValue->Get());

			int ParameterIndex;
			if (Properties->TryGetNumberField("ParameterIndex", ParameterIndex)) DynamicParameter->ParameterIndex = ParameterIndex;

			Expression = DynamicParameter;
		} else if (Type->Type == "MaterialExpressionFeatureLevelSwitch") {
			UMaterialExpressionFeatureLevelSwitch* FeatureLevelSwitch = Cast<UMaterialExpressionFeatureLevelSwitch>(Expression);

			const TSharedPtr<FJsonObject>* DefaultPtr = nullptr;
			if (Properties->TryGetObjectField("Default", DefaultPtr) && DefaultPtr != nullptr) {
				FJsonObject* DefaultObject = DefaultPtr->Get();
				FName DefaultExpressionName = GetExpressionName(DefaultObject);
				if (CreatedExpressionMap.Contains(DefaultExpressionName)) {
					FExpressionInput Default = PopulateExpressionInput(DefaultObject, *CreatedExpressionMap.Find(DefaultExpressionName));
					FeatureLevelSwitch->Default = Default;
				}
			}

			const TArray<TSharedPtr<FJsonValue>>* InputsPtr;
			if (Type->Json->TryGetArrayField("Inputs", InputsPtr)) {
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

			Expression = FeatureLevelSwitch;
		}

		AddGenerics(Parent, Expression, Properties);
		if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) FuncCasted->GetExpressionCollection().AddExpression(Expression);
		else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) MatCasted->GetExpressionCollection().AddExpression(Expression);
	}
}

void UMaterialFunctionImporter::AddComments(UObject* Parent, const TSharedPtr<FJsonObject>& Json, TMap<FName, FImportData>& Exports) {
	const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments;
	if (Json->TryGetArrayField("EditorComments", StringExpressionComments)) {
		for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
			if (ExpressionComment->IsNull()) continue;
			FName ExportName = GetExportNameOfSubobject(ExpressionComment.Get()->AsObject()->GetStringField("ObjectName"));

			const TSharedPtr<FJsonObject> Comment = Exports.Find(ExportName)->Json->GetObjectField("Properties");
			UMaterialExpressionComment* MatComment = NewObject<UMaterialExpressionComment>(Parent, UMaterialExpressionComment::StaticClass(), ExportName, RF_Transactional);

			int SizeX;
			if (Comment->TryGetNumberField("SizeX", SizeX)) MatComment->SizeX = SizeX;
			int SizeY;
			if (Comment->TryGetNumberField("SizeY", SizeY)) MatComment->SizeY = SizeY;
			FString Text;
			if (Comment->TryGetStringField("Text", Text)) MatComment->Text = Text;
			const TSharedPtr<FJsonObject>* CommentColor;
			if (Comment->TryGetObjectField("CommentColor", CommentColor)) MatComment->CommentColor = FMathUtilities::ObjectToLinearColor(CommentColor->Get());
			int FontSize;
			if (Comment->TryGetNumberField("FontSize", FontSize)) MatComment->FontSize = FontSize;
			AddGenerics(Parent, MatComment, Comment);

			if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) FuncCasted->GetExpressionCollection().AddComment(MatComment);
			else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) MatCasted->GetExpressionCollection().AddComment(MatComment);
		}
	}
}

void UMaterialFunctionImporter::AddGenerics(UObject* Parent, UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Json) {
	int MaterialExpressionEditorX;
	int MaterialExpressionEditorY;
	FString Desc;
	bool bCommentBubbleVisible;
	bool bCollapsed;
	bool bRealtimePreview;
	bool bShowOutputNameOnPin;

	if (Json->TryGetNumberField("MaterialExpressionEditorX", MaterialExpressionEditorX)) Expression->MaterialExpressionEditorX = MaterialExpressionEditorX;
	if (Json->TryGetNumberField("MaterialExpressionEditorY", MaterialExpressionEditorY)) Expression->MaterialExpressionEditorY = MaterialExpressionEditorY;

	FString MaterialExpressionGuid;
	if (Json->TryGetStringField("MaterialExpressionGuid", MaterialExpressionGuid)) Expression->MaterialExpressionGuid = FGuid(MaterialExpressionGuid);
	if (UMaterialFunction* FuncCasted = Cast<UMaterialFunction>(Parent)) Expression->Function = FuncCasted;
	else if (UMaterial* MatCasted = Cast<UMaterial>(Parent)) Expression->Material = MatCasted;
	if (Json->TryGetStringField("Desc", Desc)) Expression->Desc = Desc;
	if (Json->TryGetBoolField("bCommentBubbleVisible", bCommentBubbleVisible)) Expression->bCommentBubbleVisible = bCommentBubbleVisible;
	if (Json->TryGetBoolField("bCollapsed", bCollapsed)) Expression->bCollapsed = bCollapsed;
	if (Json->TryGetBoolField("bRealtimePreview", bRealtimePreview)) Expression->bRealtimePreview = bRealtimePreview;
	if (Json->TryGetBoolField("bShowOutputNameOnPin", bShowOutputNameOnPin)) Expression->bShowOutputNameOnPin = bShowOutputNameOnPin;

	const TArray<TSharedPtr<FJsonValue>>* OutputsPtr;
	if (Json->TryGetArrayField("Outputs", OutputsPtr)) {
		TArray<FExpressionOutput> Outputs;
		for (const TSharedPtr<FJsonValue> FunctionOutputValue : *OutputsPtr) {
			Outputs.Add(PopulateExpressionOutput(FunctionOutputValue->AsObject().Get()));
		}

		Expression->Outputs = Outputs;
	}

	if (UMaterialExpressionParameter* Parameter = Cast<UMaterialExpressionParameter>(Expression)) {
		FString ExpressionGUID;
		if (Json->TryGetStringField("ExpressionGUID", ExpressionGUID)) Parameter->ExpressionGUID = FGuid(ExpressionGUID);
		FString ParameterName;
		if (Json->TryGetStringField("ParameterName", ParameterName)) Parameter->ParameterName = FName(ParameterName);
		FString Group;
		if (Json->TryGetStringField("Group", Group)) Parameter->Group = FName(Group);
		int SortPriority;
		if (Json->TryGetNumberField("SortPriority", SortPriority)) Parameter->SortPriority = SortPriority;
	}

	if (UMaterialExpressionTextureBase* TextureBase = Cast<UMaterialExpressionTextureBase>(Expression)) {
		const TSharedPtr<FJsonObject>* TexturePtr = nullptr;
		if (Json->TryGetObjectField("Texture", TexturePtr) && TexturePtr != nullptr) {
			TextureBase->Texture = LoadObject<UTexture>(TexturePtr);
		}

		FString SamplerTypeString;
		if (Json->TryGetStringField("SamplerType", SamplerTypeString)) {
			EMaterialSamplerType SamplerType = TextureBase->SamplerType;

			if (SamplerTypeString.EndsWith("SAMPLERTYPE_Color")) SamplerType = SAMPLERTYPE_Color;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Grayscale")) SamplerType = SAMPLERTYPE_Grayscale;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Alpha")) SamplerType = SAMPLERTYPE_Alpha;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Normal")) SamplerType = SAMPLERTYPE_Normal;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Masks")) SamplerType = SAMPLERTYPE_Masks;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_DistanceFieldFont")) SamplerType = SAMPLERTYPE_DistanceFieldFont;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_LinearColor")) SamplerType = SAMPLERTYPE_LinearColor;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_LinearGrayscale")) SamplerType = SAMPLERTYPE_LinearGrayscale;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_Data")) SamplerType = SAMPLERTYPE_Data;
			else if (SamplerTypeString.EndsWith("SAMPLERTYPE_External")) SamplerType = SAMPLERTYPE_External;

			TextureBase->SamplerType = SamplerType;
		}

		bool IsDefaultMeshpaintTexture;
		if (Json->TryGetBoolField("IsDefaultMeshpaintTexture", IsDefaultMeshpaintTexture)) TextureBase->IsDefaultMeshpaintTexture = IsDefaultMeshpaintTexture;
	}
}

UMaterialExpression* UMaterialFunctionImporter::CreateEmptyExpression(UObject* Parent, const FName Name, const FName Type) const {
	if (IgnoredTypes.Contains(Type.ToString())) return nullptr;
	if (AcceptedTypes.Contains(Type.ToString())) {
		const FString Class = "/Script/Engine." + Type.ToString();
		return NewObject<UMaterialExpression>(Parent, FindObject<UClass>(nullptr, *Class), Name, RF_Transactional);
	}

	UE_LOG(LogJson, Warning, TEXT("Missing support for expression type: \"%s\""), *Type.ToString())
	return nullptr;
}

FExpressionInput UMaterialFunctionImporter::PopulateExpressionInput(const FJsonObject* JsonProperties, UMaterialExpression* Expression, const FString& Type) {
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
