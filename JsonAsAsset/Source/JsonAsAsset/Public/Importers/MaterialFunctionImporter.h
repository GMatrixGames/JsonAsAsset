// Copyright JAA Contributors 2023-2024

#pragma once

#include "Utilities/EditorGraph/MaterialGraph_Interface.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

class UMaterialFunctionImporter : public UMaterialGraph_Interface {
public:
	UMaterialFunctionImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		UMaterialGraph_Interface(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;
};
