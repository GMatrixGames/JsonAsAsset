// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/MaterialFunctionImporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComment.h"
#include "Utilities/MathUtilities.h"

bool UMaterialFunctionImporter::ImportData() {
	try {
		UMaterialFunctionFactoryNew* MaterialFunctionFactory = NewObject<UMaterialFunctionFactoryNew>();
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>(MaterialFunctionFactory->FactoryCreateNew(UMaterialFunction::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));
		MaterialFunction->StateId = FGuid(JsonObject->GetObjectField("Properties")->GetStringField("StateId"));

		FAssetRegistryModule::AssetCreated(MaterialFunction);
		if (!MaterialFunction->MarkPackageDirty()) return false;
		Package->SetDirtyFlag(true);
		MaterialFunction->PostEditChange();
		MaterialFunction->AddToRoot();

		TSharedPtr<FJsonValue> EditorOnlyData;
		TArray<TSharedPtr<FJsonObject>> Exports;
		
		for (const TSharedPtr<FJsonValue>& Jo : AllJsonObjects) {
			if (EditorOnlyData == nullptr && Jo->AsObject()->GetStringField("Type") == "MaterialFunctionEditorOnlyData") {
				EditorOnlyData = Jo;
				break;
			}

			Exports.Add(Jo->AsObject());
		}

		const TSharedPtr<FJsonObject> StringExpressionCollection = EditorOnlyData->AsObject()->GetObjectField("Properties")->GetObjectField("ExpressionCollection");

		TArray<FString> ExpressionNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressions;
		if (StringExpressionCollection->TryGetArrayField("Expressions", StringExpressions)) {
			for (const TSharedPtr<FJsonValue> Expression : *StringExpressions) {
				ExpressionNames.Add(Expression->AsObject()->GetStringField("ObjectName"));
			}
		}

		TArray<FString> ExpressionCommentNames;
		const TArray<TSharedPtr<FJsonValue>>* StringExpressionComments;
		if (StringExpressionCollection->TryGetArrayField("EditorComments", StringExpressionComments)) {
			for (const TSharedPtr<FJsonValue> ExpressionComment : *StringExpressionComments) {
				FString ExportName = GetExportNameOfSubobject(ExpressionComment.Get()->AsObject()->GetStringField("ObjectName"));

				// THIS DIES
				TSharedPtr<FJsonObject> Comment;
				for (const TSharedPtr<FJsonObject>& Object : Exports) {
					FString Test = Object->GetStringField("Name");

					if (Test == ExportName) {
						Comment = Object;
						break;
					}
				}

				UMaterialExpressionComment* MatComment = NewObject<UMaterialExpressionComment>(MaterialFunction, UMaterialExpressionComment::StaticClass(), FName(ExportName), RF_Transactional);

				int SizeX;
				int SizeY;
				int FontSize;
				int MaterialExpressionEditorX;
				int MaterialExpressionEditorY;

				FString Text;

				const TSharedPtr<FJsonObject>* CommentColor;

				if (Comment->TryGetNumberField("SizeX", SizeX)) MatComment->SizeX = SizeX;
				if (Comment->TryGetNumberField("SizeY", SizeY)) MatComment->SizeY = SizeY;
				if (Comment->TryGetStringField("Text", Text)) MatComment->Text = Text;
				if (Comment->TryGetObjectField("CommentColor", CommentColor)) MatComment->CommentColor = FMathUtilities::ObjectToLinearColor(CommentColor->Get());
				if (Comment->TryGetNumberField("FontSize", FontSize)) MatComment->FontSize = FontSize;
				if (Comment->TryGetNumberField("MaterialExpressionEditorX", MaterialExpressionEditorX)) MatComment->MaterialExpressionEditorX = MaterialExpressionEditorX;
				if (Comment->TryGetNumberField("MaterialExpressionEditorY", MaterialExpressionEditorY)) MatComment->MaterialExpressionEditorY = MaterialExpressionEditorY;
				MatComment->MaterialExpressionGuid = FGuid(Comment->GetStringField("MaterialExpressionGuid"));
				MatComment->Function = MaterialFunction;

				MaterialFunction->GetExpressionCollection().AddComment(MatComment);
			}
		}

		// MaterialFunction->GetExpressionCollection()
		// auto NewExpression = NewObject<UMaterialExpressionAdd>(MaterialFunction, UMaterialExpressionAdd::StaticClass(), FName(), RF_Transactional);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}