// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/RemoteUtilities.h"

#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformProcess.h"
#include "WindowsPlatformTime.h"
#include "Serialization/JsonSerializer.h"

TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> FRemoteUtilities::ExecuteRequestSync(TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> HttpRequest, float LoopDelay) {
	const bool bStartedRequest = HttpRequest->ProcessRequest();
	if (!bStartedRequest) {
		UE_LOG(LogJson, Error, TEXT("Failed to start HTTP Request."));
		return nullptr;
	}

	double LastTime = FPlatformTime::Seconds();
	while (EHttpRequestStatus::Processing == HttpRequest->GetStatus()) {
		const double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(LoopDelay);
	}

	return HttpRequest->GetResponse();
}
