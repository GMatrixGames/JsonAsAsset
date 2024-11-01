// Copyright JAA Contributors 2024-2025

#pragma once

#include "../Constructor/MaterialGraph.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

class UMaterialFunctionImporter : public IMaterialGraph {
public:
	UMaterialFunctionImporter(const FString& FileName, const FString& FilePath, const TSharedPtr<FJsonObject>& JsonObject, UPackage* Package, UPackage* OutermostPkg, const TArray<TSharedPtr<FJsonValue>>& AllJsonObjects):
		IMaterialGraph(FileName, FilePath, JsonObject, Package, OutermostPkg, AllJsonObjects) {
	}

	virtual bool ImportData() override;
};
